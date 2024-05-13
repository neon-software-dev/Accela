/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PhysXPhysics.h"

#include "../Metrics.h"

#include <Accela/Engine/Scene/IWorldResources.h>

#include "../Scene/MeshResources.h"

#include <Accela/Common/Assert.h>
#include <Accela/Common/BuildInfo.h>

#include <thread>

namespace Accela::Engine
{

static inline physx::PxQuat ToPhysX(const glm::quat& q) { return {q.x, q.y, q.z, q.w}; }
static inline physx::PxVec3 ToPhysX(const glm::vec3& vec) { return {vec.x, vec.y, vec.z}; }
static inline physx::PxExtendedVec3 ToPhysXExt(const glm::vec3& vec) { return {vec.x, vec.y, vec.z}; }
static inline physx::PxVec4 ToPhysX(const glm::vec4& vec) { return {vec.x, vec.y, vec.z, vec.w}; }

static inline glm::quat FromPhysX(const physx::PxQuat& q) { return {q.w, q.x, q.y, q.z}; }
static inline glm::vec3 FromPhysX(const physx::PxVec3& vec) { return {vec.x, vec.y, vec.z}; }
static inline glm::vec3 FromPhysX(const physx::PxExtendedVec3& vec) { return {vec.x, vec.y, vec.z}; }
static inline glm::vec4 FromPhysX(const physx::PxVec4& vec) { return {vec.x, vec.y, vec.z, vec.w}; }

static physx::PxShapeFlags GetShapeFlags(const ShapeUsage& usage)
{
    physx::PxShapeFlags flags = physx::PxShapeFlag::eSCENE_QUERY_SHAPE;

    switch (usage)
    {
        case ShapeUsage::Simulation:
            flags |= physx::PxShapeFlag::eSIMULATION_SHAPE;
        break;
        case ShapeUsage::Trigger:
            flags |= physx::PxShapeFlag::eTRIGGER_SHAPE;
        break;
    }

    if (Common::BuildInfo::IsDebugBuild())
    {
        flags |= physx::PxShapeFlag::eVISUALIZATION;
    }

    return flags;
}

PhysXPhysics::PhysXPhysics(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           IWorldResourcesPtr worldResources)
   : m_logger(std::move(logger))
   , m_metrics(std::move(metrics))
   , m_worldResources(std::move(worldResources))
   , m_physXLogger(m_logger)
{
    InitPhysX();
    CreateScene();
}

PhysXPhysics::~PhysXPhysics()
{
    DestroyScene();
    DestroyPhysX();
}

void PhysXPhysics::InitPhysX()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Initializing PhysX");

    m_pxFoundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        m_pxAllocator,
        m_physXLogger
    );

    // TODO Perf: What's the proper thread count value in relation to hardware threads?
    // TODO Perf: Evaluate perf of other "work wait mode" parameter values
    m_pxCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());

    m_pxPhysics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_pxFoundation,
        physx::PxTolerancesScale(),
        false, // TODO: Turn on for debug?
        nullptr
    );

    #ifdef ACCELA_USE_GPU_CUDA
        physx::PxCudaContextManagerDesc cudaContextManagerDesc{};
        m_pxCudaContextManager = PxCreateCudaContextManager(*m_pxFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
    #endif
}

void PhysXPhysics::DestroyPhysX()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying PhysX");

    PX_RELEASE(m_pxCudaContextManager)
    PX_RELEASE(m_pxPhysics)
    PX_RELEASE(m_pxCpuDispatcher)
    PX_RELEASE(m_pxFoundation)

    SyncMetrics();
}

void PhysXPhysics::CreateScene()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Creating Scene");

    physx::PxSceneDesc sceneDesc(m_pxPhysics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher	= m_pxCpuDispatcher;
    sceneDesc.filterShader	= physx::PxDefaultSimulationFilterShader;
    #ifdef ACCELA_USE_GPU_CUDA
        // TODO Perf: Tweak values in sceneDesc.gpuDynamicsConfig
        sceneDesc.cudaContextManager = m_pxCudaContextManager;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
        sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
    #endif
    m_pxScene = m_pxPhysics->createScene(sceneDesc);
    m_pxScene->setFlag(physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS, true);
    m_pxScene->setSimulationEventCallback(this);

    m_pxControllerManager = PxCreateControllerManager(*m_pxScene);
}

void PhysXPhysics::DestroyScene()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying Scene");

    for (auto& it : m_playerControllers)
    {
        PX_RELEASE(it.second.pPxController)
    }
    m_playerControllers.clear();
    m_physXActorToPlayerController.clear();

    while (!m_entityToRigidBody.empty())
    {
        DestroyRigidBody(m_entityToRigidBody.cbegin()->first);
    }
    m_physXActorToEntity.clear();

    PX_RELEASE(m_pxControllerManager)
    PX_RELEASE(m_pxScene)

    SyncMetrics();
}

void PhysXPhysics::SimulationStep(unsigned int timeStep)
{
    assert(m_pxScene != nullptr);

    ApplyPlayerControllerMovements(timeStep);

    m_pxScene->simulate((float)timeStep / 1000.0f);
    // TODO Perf: Can we make use of the simulate threading?
    m_pxScene->fetchResults(true);

    SyncBodyDataFromPhysX();

    if (Common::BuildInfo::IsDebugBuild()) { DebugCheckResources(); }
}

void PhysXPhysics::SyncBodyDataFromPhysX()
{
    physx::PxU32 numActiveActors{0};
    const auto activeActors = m_pxScene->getActiveActors(numActiveActors);

    for (uint32_t x = 0; x < numActiveActors; ++x)
    {
        const auto it = m_physXActorToEntity.find(activeActors[x]);
        if (it == m_physXActorToEntity.cend()) { continue; }

        const auto it2 = m_entityToRigidBody.find(it->second);
        if (it2 == m_entityToRigidBody.cend()) { continue; }

        //
        // Sync actor data
        //
        const auto globalPose = it2->second.pRigidActor->getGlobalPose();
        it2->second.data.actor.position = FromPhysX(globalPose.p);
        it2->second.data.actor.orientation = FromPhysX(globalPose.q);

        //
        // Update dirty state
        //
        it2->second.isDirty = true;
    }
}

void PhysXPhysics::ApplyPlayerControllerMovements(unsigned int timeStep)
{
    for (auto& characterControllerIt : m_playerControllers)
    {
        auto& controller = characterControllerIt.second;

        if (!controller.movementCommand)
        {
            controller.msSinceLastUpdate += timeStep;
            continue;
        }

        characterControllerIt.second.pPxController->move(
            ToPhysX(controller.movementCommand->movement),
            controller.movementCommand->minDistance,
            (float)controller.msSinceLastUpdate / 1000.0f,
            physx::PxControllerFilters{}
        );

        controller.msSinceLastUpdate = 0;
        controller.movementCommand = std::nullopt;
    }
}

std::optional<std::pair<RigidBody, bool>> PhysXPhysics::GetRigidBody(const EntityId& eid)
{
    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        return std::nullopt;
    }

    return std::make_pair(it->second.data, it->second.isDirty);
}

void PhysXPhysics::MarkBodiesClean()
{
    for (auto& rigidActor : m_entityToRigidBody)
    {
        rigidActor.second.isDirty = false;
    }
}

std::queue<PhysicsTriggerEvent> PhysXPhysics::PopTriggerEvents()
{
    auto triggerEvents = m_triggerEvents;
    m_triggerEvents = {};
    return triggerEvents;
}

bool PhysXPhysics::CreateRigidBody(const EntityId& eid, const RigidBody& data)
{
    if (m_entityToRigidBody.contains(eid))
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreateRigidBody: Entity with id already exists: {}", eid);
        return false;
    }

    //
    // Create PhysX Actor
    //
    auto pRigidActor = CreateRigidActor(data.body);
    if (pRigidActor == nullptr) { return false; }

    SetPhysXRigidBodyDataFrom(pRigidActor, data.actor, data.body);

    //
    // Create Physx Shapes+Materials
    //
    std::vector<std::pair<physx::PxShape*, physx::PxMaterial*>> shapes;

    for (const auto& shape : data.actor.shapes)
    {
        //
        // Create PhysX Material
        //
        auto pMaterial = CreateMaterial(shape.material);
        if (pMaterial == nullptr) { return false; }

        //
        // Create PhysX Shape
        //
        auto pShape = CreateShape(shape, pMaterial);
        if (pShape == nullptr) { return false; }

        //
        // Attach shape to the actor
        //
        pRigidActor->attachShape(*pShape);

        shapes.emplace_back(pShape, pMaterial);
    }

    //
    // Configure
    //
    m_pxScene->addActor(*pRigidActor);

    //
    // Record
    //
    PhysXRigidBody rigidBody(data, pRigidActor, shapes);

    m_entityToRigidBody.insert({eid, rigidBody});
    m_physXActorToEntity.insert({pRigidActor, eid});

    SyncMetrics();
    return true;
}

bool PhysXPhysics::UpdateRigidBody(const EntityId& eid, const RigidBody& data)
{
    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::UpdateRigidBody: No such entity: {}", eid);
        return false;
    }

    //
    // Rigid Body Data
    //

    // Sync the PhysX rigid actor's physics data from the component data
    SetPhysXRigidBodyDataFrom(it->second.pRigidActor, data.actor, data.body);

    it->second.data = data;

    //
    // RigidBodyShape
    //

    // TODO Perf: Only destroy and recreate shapes if something changed

    // Remove and free any previous shapes+materials the actor might have had
    for (auto& shapeIt : it->second.shapes)
    {
        // Detach the shape from the actor
        it->second.pRigidActor->detachShape(*shapeIt.first);

        // Release the shape and its material
        PX_RELEASE(shapeIt.second)
        PX_RELEASE(shapeIt.first)
    }

    it->second.shapes.clear();

    // Create new PhysX shapes
    for (const auto& shape : data.actor.shapes)
    {
        auto* pMaterial = CreateMaterial(shape.material);
        if (pMaterial == nullptr)
        {
            m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::UpdateRigidBody: Failed to create shape material");
            return false;
        }

        auto* pShape = CreateShape(shape, pMaterial);
        if (pShape == nullptr)
        {
            m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::UpdateRigidBody: Failed to create shape");
            return false;
        }

        // Add the shape to the actor
        it->second.pRigidActor->attachShape(*pShape);

        it->second.shapes.emplace_back(pShape, pMaterial);
    }

    return true;
}

physx::PxRigidActor* PhysXPhysics::CreateRigidActor(const RigidBodyData& body)
{
    assert(m_pxPhysics != nullptr);

    switch (body.type)
    {
        case RigidBodyType::Static:
            return m_pxPhysics->createRigidStatic(physx::PxTransform(physx::PxIDENTITY{}));
        case RigidBodyType::Dynamic:
            return m_pxPhysics->createRigidDynamic(physx::PxTransform(physx::PxIDENTITY{}));
        case RigidBodyType::Kinematic:
        {
            auto pRigidDynamic = m_pxPhysics->createRigidDynamic(physx::PxTransform(physx::PxIDENTITY{}));
            pRigidDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            return pRigidDynamic;
        }
    }

    assert(false);
    return nullptr;
}

void PhysXPhysics::SetPhysXRigidBodyDataFrom(physx::PxRigidActor* pRigidActor, const RigidActorData& actor, const RigidBodyData& body)
{
    //
    // RigidActor
    //
    pRigidActor->setGlobalPose(physx::PxTransform(ToPhysX(actor.position), ToPhysX(actor.orientation)));

    //
    // RigidBody
    //
    auto* pRigidBody = GetAsRigidBody(pRigidActor);
    if (pRigidBody != nullptr)
    {
        pRigidBody->setMass(body.mass);
    }

    //
    // RigidBody SubData
    //
    if (body.type == RigidBodyType::Dynamic || body.type == RigidBodyType::Kinematic)
    {
        auto* pRigidDynamic = GetAsRigidDynamic(pRigidActor);
        const auto& dynamicData = std::get<RigidBodyDynamicData>(body.subData);

        if (pRigidDynamic != nullptr)
        {
            pRigidDynamic->setLinearVelocity(ToPhysX(dynamicData.linearVelocity));
            pRigidDynamic->setLinearDamping(dynamicData.linearDamping);
            pRigidDynamic->setAngularDamping(dynamicData.angularDamping);

            physx::PxRigidDynamicLockFlags lockFlags{0};
            if (!dynamicData.axisMotionAllowed[0]) { lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X; }
            if (!dynamicData.axisMotionAllowed[1]) { lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y; }
            if (!dynamicData.axisMotionAllowed[2]) { lockFlags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z; }
            pRigidDynamic->setRigidDynamicLockFlags(lockFlags);
        }
    }
}

physx::PxMaterial* PhysXPhysics::CreateMaterial(const MaterialData& material)
{
    assert(m_pxPhysics != nullptr);

    auto pMaterial = m_pxPhysics->createMaterial(
        material.staticFriction,
        material.dynamicFriction,
        material.restitution
    );
    if (pMaterial == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "CreateMaterial: Failed to create material");
        return nullptr;
    }

    return pMaterial;
}

physx::PxShape* PhysXPhysics::CreateShape(const ShapeData& shape, physx::PxMaterial* pMaterial)
{
    assert(m_pxPhysics != nullptr);

    // Internal translation adjustments of the shape's model-space position and orientation.
    // Is in addition to any adjustment that was provided in boundsComponent & transformComponent
    auto localPositionAdjustment = glm::vec3(0,0,0);
    auto localOrientationAdjustment = glm::quat(glm::identity<glm::quat>());

    physx::PxShape* pShape{nullptr};

    if (std::holds_alternative<Bounds_AABB>(shape.bounds))
    {
        pShape = CreateShape_AABB(shape, pMaterial, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_Capsule>(shape.bounds))
    {
        pShape = CreateShape_Capsule(shape, pMaterial, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_Sphere>(shape.bounds))
    {
        pShape = CreateShape_Sphere(shape, pMaterial, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_StaticMesh>(shape.bounds))
    {
        pShape = CreateShape_StaticMesh(shape, pMaterial, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_HeightMap>(shape.bounds))
    {
        pShape = CreateShape_HeightMap(shape, pMaterial, localPositionAdjustment, localOrientationAdjustment);
    }

    if (pShape == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "CreateRigidActorShape: Failed to create shape");
        return nullptr;
    }

    // Local transform
    auto localTransform = physx::PxTransform(physx::PxIDENTITY{});
    localTransform.p = ToPhysX(shape.localTransform + localPositionAdjustment);
    localTransform.q = ToPhysX(shape.localOrientation * localOrientationAdjustment);
    pShape->setLocalPose(localTransform);

    return pShape;
}

physx::PxShape* PhysXPhysics::CreateShape_AABB(const ShapeData& shape,
                                               physx::PxMaterial* pMaterial,
                                               glm::vec3&)
{
    const auto boundsAABB = std::get<Bounds_AABB>(shape.bounds);

    auto boxSize = boundsAABB.max - boundsAABB.min;
    boxSize *= shape.scale;

    const auto boxSizeHalfExtents = boxSize / 2.0f;  // PxBoxGeometry requires "half extents"

    return m_pxPhysics->createShape(
        physx::PxBoxGeometry(ToPhysX(boxSizeHalfExtents)),
        *pMaterial,
        true,
        GetShapeFlags(shape.usage)
    );
}

physx::PxShape* PhysXPhysics::CreateShape_Capsule(const ShapeData& shape,
                                                  physx::PxMaterial* pMaterial,
                                                  glm::vec3&)
{
    const auto boundsCapsule = std::get<Bounds_Capsule>(shape.bounds);

    const auto transformScale = shape.scale;
    const bool horizScaleIsUniform = transformScale.x == transformScale.z;

    if (!Common::Assert(horizScaleIsUniform, m_logger,
        "CreateRigidActorShape_Capsule: Entity has non-uniform x/z scale applied"))
    {
        return nullptr;
    }

    const float radiusScaled = boundsCapsule.radius * transformScale.x;
    const float heightScaled = boundsCapsule.height * transformScale.y;

    return m_pxPhysics->createShape(
        physx::PxCapsuleGeometry(radiusScaled, heightScaled / 2.0f),
        *pMaterial,
        true,
        GetShapeFlags(shape.usage)
    );
}

physx::PxShape* PhysXPhysics::CreateShape_Sphere(const ShapeData& shape,
                                                 physx::PxMaterial* pMaterial,
                                                 glm::vec3&)
{
    const auto boundsSphere = std::get<Bounds_Sphere>(shape.bounds);

    const auto transformScale = shape.scale;
    const bool scaleIsUniform = transformScale.x == transformScale.y && transformScale.x == transformScale.z;

    if (!Common::Assert(scaleIsUniform, m_logger,
        "CreateRigidActorShape_Sphere: Entity has a non-uniform scale applied"))
    {
        return nullptr;
    }

    const float radiusScaled = boundsSphere.radius * transformScale.x;

    return m_pxPhysics->createShape(
        physx::PxSphereGeometry(radiusScaled),
        *pMaterial,
        true,
        GetShapeFlags(shape.usage)
    );
}

physx::PxShape* PhysXPhysics::CreateShape_StaticMesh(const ShapeData& shape,
                                                     physx::PxMaterial* pMaterial,
                                                     glm::vec3&)
{
    const auto boundsStaticMesh = std::get<Bounds_StaticMesh>(shape.bounds);

    const auto staticMeshDataOpt = std::dynamic_pointer_cast<MeshResources>(m_worldResources->Meshes())
        ->GetStaticMeshData(boundsStaticMesh.staticMeshId);
    if (!staticMeshDataOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateRigidActorShape_StaticMesh: No such static mesh found, id: {}", boundsStaticMesh.staticMeshId.id);
        return nullptr;
    }

    std::vector<physx::PxVec3> pxVertices;
    pxVertices.reserve((*staticMeshDataOpt)->vertices.size());
    for (const auto& vertex : (*staticMeshDataOpt)->vertices)
    {
        pxVertices.push_back(ToPhysX(vertex.position));
    }

    std::vector<physx::PxU32> pxIndices;
    pxIndices.reserve((*staticMeshDataOpt)->indices.size());
    for (const auto& index : (*staticMeshDataOpt)->indices)
    {
        pxIndices.push_back(index);
    }

    physx::PxTolerancesScale scale{};
    physx::PxCookingParams params(scale);
    params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
    params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

    physx::PxTriangleMeshDesc meshDesc;
    meshDesc.points.count           = pxVertices.size();
    meshDesc.points.stride          = sizeof(physx::PxVec3);
    meshDesc.points.data            = pxVertices.data();

    meshDesc.triangles.count        = pxIndices.size() / 3;
    meshDesc.triangles.stride       = 3 * sizeof(physx::PxU32);
    meshDesc.triangles.data         = pxIndices.data();

    if (Common::BuildInfo::IsDebugBuild())
    {
        const bool meshValidationResult = PxValidateTriangleMesh(params, meshDesc);
        if (!meshValidationResult)
        {
            m_logger->Log(Common::LogLevel::Error, "CreateRigidActorShape_StaticMesh: Mesh failed validation");
            return nullptr;
        }
    }

    auto* pTriangleMesh = PxCreateTriangleMesh(
        params,
        meshDesc,
        m_pxPhysics->getPhysicsInsertionCallback()
    );
    if (pTriangleMesh == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "CreateRigidActorShape_StaticMesh: Failed to create triangle mesh");
        return nullptr;
    }

    return m_pxPhysics->createShape(
        physx::PxTriangleMeshGeometry(pTriangleMesh, ToPhysX(shape.scale)),
        *pMaterial,
        true,
        GetShapeFlags(shape.usage)
    );
}

physx::PxShape* PhysXPhysics::CreateShape_HeightMap(const ShapeData& shape,
                                                    physx::PxMaterial* pMaterial,
                                                    glm::vec3& localPositionAdjustment,
                                                    glm::quat& localOrientationAdjustment)
{
    const auto boundsHeightMap = std::get<Bounds_HeightMap>(shape.bounds);

    const auto heightMapDataOpt = std::dynamic_pointer_cast<MeshResources>(m_worldResources->Meshes())
        ->GetHeightMapData(boundsHeightMap.heightMapMeshId);
    if (!heightMapDataOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateRigidActorShape_HeightMap: No such height map mesh found, id: {}", boundsHeightMap.heightMapMeshId.id);
        return nullptr;
    }

    const auto& heightMapData = *heightMapDataOpt;

    // How to scale the height map data points to mesh model coordinates
    const glm::vec3 scaleToMeshSize(
        (float)heightMapData->meshSize_worldSpace.w / ((float)heightMapData->dataSize.w - 1),
        1.0f,
        (float)heightMapData->meshSize_worldSpace.h / ((float)heightMapData->dataSize.h - 1)
    );

    // Scale the data points to model points, then scale by the model's object scale
    const glm::vec3 colliderScale = shape.scale * scaleToMeshSize;

    ////////////////////////

    // https://nvidia-omniverse.github.io/PhysX/physx/5.3.1/docs/Geometry.html#quantizing-heightfield-samples
    physx::PxReal pxMinHeightFieldYScale{0.0f};
    { using namespace physx; pxMinHeightFieldYScale = PX_MIN_HEIGHTFIELD_Y_SCALE; }

    const physx::PxReal deltaHeight = (physx::PxReal)heightMapData->maxValue - (physx::PxReal)heightMapData->minValue;
    const auto quantization = (physx::PxReal)0x7fff;
    const physx::PxReal heightScale = physx::PxMax(deltaHeight / quantization, pxMinHeightFieldYScale);

    std::vector<physx::PxHeightFieldSample> heightFieldSamples;
    heightFieldSamples.reserve((int)heightMapData->dataSize.w * (int)heightMapData->dataSize.h);

    for (unsigned int x = 0; x < heightMapData->dataSize.w; ++x)
    {
        for (unsigned int y = 0; y < heightMapData->dataSize.h; ++y)
        {
            // Even though physx documentation says it builds the height map from the far/left corner first, and that's the
            // format the height map data is in, only if we build the physx samples from the front/left corner first does
            // it work properly, so that's why the sample height value is inverted:
            const double& sampleRawValue = heightMapData->data[(heightMapData->dataSize.h - 1 - y) + (x * heightMapData->dataSize.w)];

            const physx::PxReal quantizedHeight = physx::PxI16(quantization * ((sampleRawValue - (physx::PxReal)heightMapData->minValue) / deltaHeight));

            physx::PxHeightFieldSample sample{};
            sample.height = (physx::PxI16)quantizedHeight;
            sample.materialIndex0 = 0;
            sample.materialIndex1 = 0;
            sample.clearTessFlag();

            heightFieldSamples.push_back(sample);
        }
    }

    physx::PxHeightFieldDesc hfDesc;
    hfDesc.format             = physx::PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns          = (int)heightMapData->dataSize.w;
    hfDesc.nbRows             = (int)heightMapData->dataSize.h;
    hfDesc.samples.data       = heightFieldSamples.data();
    hfDesc.samples.stride     = sizeof(physx::PxHeightFieldSample);

    physx::PxHeightField* pHeightField = PxCreateHeightField(hfDesc,m_pxPhysics->getPhysicsInsertionCallback());

    physx::PxHeightFieldGeometry heightFieldGeometry(
        pHeightField,
        physx::PxMeshGeometryFlags(),
        deltaHeight != 0.0f ? heightScale : 1.0f,
        colliderScale.x,
        colliderScale.z
    );

    // PhysX creates the shape rotated 90 degrees the wrong way, correct for this
    localOrientationAdjustment = glm::angleAxis(glm::radians(-90.0f), glm::vec3{0,1,0});

    // PhysX aligns height maps with the back-left corner the origin, but we want the center of the height map
    // to be the origin, so offset it by half its height/width to adjust it. Note that after the orientation change
    // from above, we need to translate it differently, as the rotation is around that back-left corner, shifting the
    // height map from in front of and to the right of 0,0 to being in front of and to the left of 0,0.
    localPositionAdjustment = shape.scale * glm::vec3{
        ((float)heightMapData->meshSize_worldSpace.w / 2.0f),
        heightMapData->minValue, // Adjust upwards by minValue so that minValue points are at minValue height above 0
        -((float)heightMapData->meshSize_worldSpace.h / 2.0f),
    };

    return m_pxPhysics->createShape(
        heightFieldGeometry,
        *pMaterial,
        true,
        GetShapeFlags(shape.usage)
    );
}

void PhysXPhysics::DestroyRigidBody(const EntityId& eid)
{
    m_logger->Log(Common::LogLevel::Debug, "PhysXPhysics::DestroyRigidBody: Destroying rigid body for entity: {}", eid);

    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        return;
    }

    // Detach and destroy actor shapes+materials
    for (auto& shapeIt : it->second.shapes)
    {
        it->second.pRigidActor->detachShape(*shapeIt.first);
        PX_RELEASE(shapeIt.second)
        PX_RELEASE(shapeIt.first)
    }

    // Remove the actor itself from the scene and destroy it
    m_pxScene->removeActor(*it->second.pRigidActor);
    PX_RELEASE(it->second.pRigidActor)
    it->second.pRigidActor = nullptr;

    // Remove our records of the body/entity
    m_entityToRigidBody.erase(it);
    m_physXActorToEntity.clear();

    SyncMetrics();
}

bool PhysXPhysics::CreatePlayerController(const std::string& name,
                                          const glm::vec3& position,
                                          const float& radius,
                                          const float& height,
                                          const PhysicsMaterial& material)
{
    m_logger->Log(Common::LogLevel::Info,
      "PhysXPhysics::CreatePlayerController: Creating player controller: {}", name);

    if (m_playerControllers.contains(name))
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreatePlayerController: Player controller already existed: {}", name);
        return false;
    }

    physx::PxCapsuleControllerDesc desc{};
    desc.radius = radius;
    desc.height = height;
    desc.material = m_pxPhysics->createMaterial(material.staticFriction, material.dynamicFriction, material.restitution);
    desc.position = ToPhysXExt(position);

    auto* pxPlayerController = m_pxControllerManager->createController(desc);
    if (pxPlayerController == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error,
          "PhysXPhysics::CreatePlayerController: Failed to create PhysX controller manager: {}", name);
        PX_RELEASE(desc.material)
        return false;
    }

    m_playerControllers.insert({name, PhysXPlayerController(pxPlayerController, desc.material)});
    m_physXActorToPlayerController.insert({pxPlayerController->getActor(), name});

    return true;
}

std::optional<glm::vec3> PhysXPhysics::GetPlayerControllerPosition(const std::string& name)
{
    const auto it = m_playerControllers.find(name);
    if (it == m_playerControllers.cend())
    {
        return std::nullopt;
    }

    return FromPhysX(it->second.pPxController->getPosition());
}

std::optional<PlayerControllerState> PhysXPhysics::GetPlayerControllerState(const std::string& name)
{
    const auto it = m_playerControllers.find(name);
    if (it == m_playerControllers.cend())
    {
        return std::nullopt;
    }

    physx::PxControllerState pxControllerState{};
    it->second.pPxController->getState(pxControllerState);

    PlayerControllerState playerControllerState{};
    playerControllerState.collisionAbove = pxControllerState.collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP;
    playerControllerState.collisionBelow = pxControllerState.collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN;

    return playerControllerState;
}

bool PhysXPhysics::SetPlayerControllerMovement(const std::string& name,
                                               const glm::vec3& movement,
                                               const float& minDistance)
{
    const auto it = m_playerControllers.find(name);
    if (it == m_playerControllers.cend())
    {
        return false;
    }

    it->second.movementCommand = PhysxMovement(movement, minDistance);

    return true;
}

void PhysXPhysics::DestroyPlayerController(const std::string& name)
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Destroying player controller: {}", name);

    const auto it = m_playerControllers.find(name);
    if (it == m_playerControllers.cend())
    {
        return;
    }

    m_physXActorToPlayerController.erase((physx::PxActor*)it->second.pPxController->getActor());

    PX_RELEASE(it->second.pMaterial)
    PX_RELEASE(it->second.pPxController)

    m_playerControllers.erase(it);
}

void PhysXPhysics::ClearAll()
{
    m_logger->Log(Common::LogLevel::Info, "PhysXPhysics: Clearing All");

    DestroyScene();
    CreateScene();
}

void PhysXPhysics::EnableDebugRenderOutput(bool enable)
{
    assert(m_pxScene != nullptr);

    const float val = enable ? 1.0f : 0.0f;

    m_pxScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, val);
    m_pxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, val);
}

std::vector<Render::Triangle> PhysXPhysics::GetDebugTriangles() const
{
    assert(m_pxScene != nullptr);

    std::vector<Render::Triangle> triangles;

    const physx::PxRenderBuffer& rb = m_pxScene->getRenderBuffer();

    for (physx::PxU32 x = 0; x < rb.getNbLines(); ++x)
    {
        // TODO: Raw line rendering rather than creating fake triangles from lines
        // TODO: Make third point orthogonal so there isn't winding order culling issues
        const physx::PxDebugLine& line = rb.getLines()[x];
        triangles.emplace_back(FromPhysX(line.pos0), FromPhysX(line.pos1), FromPhysX(line.pos0) + glm::vec3(0,0.001f,0));
    }

    for (physx::PxU32 x = 0; x < rb.getNbTriangles(); ++x)
    {
        const physx::PxDebugTriangle& triangle = rb.getTriangles()[x];
        triangles.emplace_back(FromPhysX(triangle.pos0), FromPhysX(triangle.pos1), FromPhysX(triangle.pos2));
    }

    return triangles;
}

bool PhysXPhysics::ApplyRigidBodyLocalForce(const EntityId& eid, const glm::vec3& force)
{
    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        return false;
    }

    auto* pRigidDynamic = GetAsRigidBody(it->second.pRigidActor);
    pRigidDynamic->addForce(ToPhysX(force), physx::PxForceMode::eIMPULSE);

    return true;
}

std::vector<RaycastResult> PhysXPhysics::RaycastForCollisions(const glm::vec3& rayStart_worldSpace,
                                                              const glm::vec3& rayEnd_worldSpace) const
{
    // TODO!
    (void)rayStart_worldSpace;
    (void)rayEnd_worldSpace;

    return std::vector<RaycastResult>{};
}

physx::PxRigidBody* PhysXPhysics::GetAsRigidBody(const physx::PxRigidActor* pRigidActor)
{
    const auto pxActorType = pRigidActor->getType();

    if (pxActorType != physx::PxActorType::eRIGID_STATIC && pxActorType != physx::PxActorType::eRIGID_DYNAMIC)
    {
        return nullptr;
    }

    return (physx::PxRigidBody*)(pRigidActor);
}

physx::PxRigidDynamic* PhysXPhysics::GetAsRigidDynamic(const physx::PxRigidActor* pRigidActor)
{
    if (pRigidActor->getType() != physx::PxActorType::eRIGID_DYNAMIC) { return nullptr; }

    return (physx::PxRigidDynamic*)(pRigidActor);
}

void PhysXPhysics::SyncMetrics()
{
    m_metrics->SetCounterValue(Engine_Physics_Internal_Bodies_Count, m_entityToRigidBody.size());

    std::size_t staticBodiesCount = 0;
    std::size_t rigidBodiesCount = 0;

    if (m_pxScene != nullptr)
    {
        staticBodiesCount = m_pxScene->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC);
        rigidBodiesCount = m_pxScene->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC);
    }

    m_metrics->SetCounterValue(Engine_Physics_Static_Bodies_Count, staticBodiesCount);
    m_metrics->SetCounterValue(Engine_Dynamic_Rigid_Bodies_Count, rigidBodiesCount);
}

void PhysXPhysics::DebugCheckResources()
{
    //
    // One actor should exist for each rigid body and player controller
    //
    const auto numActors = m_pxScene->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC | physx::PxActorTypeFlag::eRIGID_DYNAMIC);
    const bool actorCountMatches = numActors == (m_entityToRigidBody.size() + m_playerControllers.size());
    Common::Assert(actorCountMatches, m_logger, "PhysXPhysics::DebugCheckResources: Actor count didn't match");

    //
    // One material should exist for each rigid body shape and player controller
    //
    std::size_t rigidBodyShapes = 0;

    for (const auto& it : m_entityToRigidBody)
    {
        rigidBodyShapes += it.second.shapes.size();
    }

    const bool materialCountMatches = m_pxPhysics->getNbMaterials() == (rigidBodyShapes + m_playerControllers.size());
    Common::Assert(materialCountMatches, m_logger, "PhysXPhysics::DebugCheckResources: Material count didn't match");

    //
    // Our entity map sizes should match
    //
    Common::Assert(m_entityToRigidBody.size() == m_physXActorToEntity.size(), m_logger,
       "PhysXPhysics::DebugCheckResources: Entity map counts don't match");

    Common::Assert(m_playerControllers.size() == m_physXActorToPlayerController.size(), m_logger,
       "PhysXPhysics::DebugCheckResources: Player map counts don't match");
}

void PhysXPhysics::onConstraintBreak(physx::PxConstraintInfo *constraints, physx::PxU32 count)
{
    (void)constraints;
    (void)count;
}

void PhysXPhysics::onWake(physx::PxActor **actors, physx::PxU32 count)
{
    (void)actors;
    (void)count;
}

void PhysXPhysics::onSleep(physx::PxActor **actors, physx::PxU32 count)
{
    (void)actors;
    (void)count;
}

void PhysXPhysics::onContact(const physx::PxContactPairHeader& pairHeader,
                             const physx::PxContactPair *pairs,
                             physx::PxU32 nbPairs)
{
    (void)pairHeader;
    (void)pairs;
    (void)nbPairs;
}

void PhysXPhysics::onTrigger(physx::PxTriggerPair *pairs, physx::PxU32 count)
{
    for (unsigned int x = 0; x < count; ++x)
    {
        const auto& triggerPair = pairs[x];

        const auto touchType = triggerPair.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND ?
                               PhysicsTriggerEvent::Type::TouchFound :
                               PhysicsTriggerEvent::Type::TouchLost;

        const auto itTrigger = m_physXActorToEntity.find(triggerPair.triggerActor);
        if (itTrigger == m_physXActorToEntity.cend())
        {
            m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::onTrigger: Trigger entity doesn't exist");
            continue;
        }

        // If the touching object is a rigid body
        const auto itOtherBody = m_physXActorToEntity.find(triggerPair.otherActor);
        if (itOtherBody != m_physXActorToEntity.cend())
        {
            m_triggerEvents.emplace(touchType, itTrigger->second, itOtherBody->second);
            continue;
        }

        // If the touching object is a player controller
        const auto itOtherPlayer = m_physXActorToPlayerController.find(triggerPair.otherActor);
        if (itOtherPlayer != m_physXActorToPlayerController.cend())
        {
            m_triggerEvents.emplace(touchType, itTrigger->second, itOtherPlayer->second);
            continue;
        }

        m_logger->Log(Common::LogLevel::Error, "PhysXPhysics::onTrigger: Other actor doesn't exist");
    }
}

void PhysXPhysics::onAdvance(const physx::PxRigidBody *const *bodyBuffer,
                             const physx::PxTransform *poseBuffer,
                             const physx::PxU32 count)
{
    (void)bodyBuffer;
    (void)poseBuffer;
    (void)count;
}

}
