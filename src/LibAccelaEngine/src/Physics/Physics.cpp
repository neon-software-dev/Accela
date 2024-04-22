/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Physics.h"

#include "../Metrics.h"
#include "../Scene/WorldResources.h"

#include <Accela/Common/Assert.h>

#include <cassert>
#include <vector>
#include <algorithm>
#include <ranges>

namespace Accela::Engine
{

static inline reactphysics3d::Vector3 ToRP3D(const glm::vec3& vec)
{
    return {vec.x, vec.y, vec.z};
}

static inline glm::vec3 FromRP3D(const reactphysics3d::Vector3& vec)
{
    return {vec.x, vec.y, vec.z};
}

static inline reactphysics3d::Quaternion ToRP3D(const glm::quat& quat)
{
    return {quat.x, quat.y, quat.z, quat.w};
}

static inline glm::quat FromRP3D(const reactphysics3d::Quaternion& quat)
{
    return {quat.w, quat.x, quat.y, quat.z};
}

void EnableRP3DLogging(reactphysics3d::PhysicsCommon& physicsCommon)
{
    auto* rp3dLogger = physicsCommon.createDefaultLogger();
    const auto logLevel = static_cast<unsigned int>(
        static_cast<unsigned int>(reactphysics3d::Logger::Level::Warning) |
        static_cast<unsigned int>(reactphysics3d::Logger::Level::Error) |
        static_cast<unsigned int>(reactphysics3d::Logger::Level::Information)
    );
    rp3dLogger->addStreamDestination(std::cout, logLevel, reactphysics3d::DefaultLogger::Format::Text);
    physicsCommon.setLogger(rp3dLogger);
}

Physics::Physics(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics, std::shared_ptr<WorldResources> worldResources)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_worldResources(std::move(worldResources))
{
    //EnableRP3DLogging(m_physicsCommon);

    reactphysics3d::PhysicsWorld::WorldSettings worldSettings{};

    m_pPhysicsWorld = m_physicsCommon.createPhysicsWorld(worldSettings);

    SyncMetrics();
}

Physics::~Physics()
{
    DestroyAll();
}

void Physics::SimulationStep(unsigned int timeStep)
{
    m_pPhysicsWorld->update((float)timeStep / 1000.0f);
}

void Physics::PostSimulationSyncRigidBodyEntity(const EntityId& eid,
                                                PhysicsComponent& physicsComponent,
                                                TransformComponent& transformComponent)
{
    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        return;
    }

    auto* pBody = it->second.pBody;

    //
    // Sync latest physics state to the entity
    //
    physicsComponent.mass = pBody->getMass();
    physicsComponent.linearVelocity = FromRP3D(pBody->getLinearVelocity());

    //
    // Sync latest transform state to the entity
    //
    auto rp3dTransform = pBody->getTransform();

    transformComponent.SetPosition(FromRP3D(rp3dTransform.getPosition()));
    transformComponent.SetOrientation(FromRP3D(rp3dTransform.getOrientation()));
}

void Physics::CreateRigidBodyFromEntity(const EntityId& eid,
                                        const PhysicsComponent& physicsComponent,
                                        const TransformComponent& transformComponent,
                                        const BoundsComponent& boundsComponent)
{
    assert(m_pPhysicsWorld != nullptr);

    //
    // Create a rigid body
    //
    auto* pBody = m_pPhysicsWorld->createRigidBody(GetRP3DTransform(transformComponent));

    switch (physicsComponent.bodyType)
    {
        case PhysicsBodyType::Static:
            pBody->setType(reactphysics3d::BodyType::STATIC);
        break;
        case PhysicsBodyType::Kinematic:
            pBody->setType(reactphysics3d::BodyType::KINEMATIC);
        break;
        case PhysicsBodyType::Dynamic:
            pBody->setType(reactphysics3d::BodyType::DYNAMIC);
        break;
    }

    SyncRigidBodyData(pBody, physicsComponent, transformComponent);

    //
    // Add a collider to the body
    //
    const auto pCollider = AddRigidCollider(pBody, physicsComponent, boundsComponent, transformComponent);
    if (!pCollider)
    {
        m_logger->Log(Common::LogLevel::Error, "CreateRigidBodyFromEntity: Failed to add rigid collider");
        m_pPhysicsWorld->destroyRigidBody(pBody);
        return;
    }

    //
    // Update State
    //
    m_entityToRigidBody.insert(std::make_pair(eid, RigidBody{pBody, *pCollider}));
    m_bodyToEntity.insert(std::make_pair(pBody, eid));
    SyncMetrics();
}

void Physics::UpdateRigidBodyFromEntity(const EntityId& eid,
                                        const PhysicsComponent& physicsComponent,
                                        const TransformComponent& transformComponent)
{
    const auto bodyIt = m_entityToRigidBody.find(eid);
    if (bodyIt == m_entityToRigidBody.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "Physics::UpdatePhysicsBodyFromEntity: Asked to update body but one doesn't exist, eid: ", eid);
        return;
    }

    SyncRigidBodyData(bodyIt->second.pBody, physicsComponent, transformComponent);

    // TODO: Collider scale isn't updated if transform scale is changed
    // TODO: Support updating collider bounds/shape in general
    // TODO: Investigate: performance concerns around recreating collider when
    //  physics/transform components are updated
}

void Physics::SyncRigidBodyData(reactphysics3d::RigidBody* pBody,
                                const PhysicsComponent& physicsComponent,
                                const TransformComponent& transformComponent)
{
    //
    // Physics Properties
    //
    pBody->setMass(physicsComponent.mass);
    pBody->setLinearVelocity(ToRP3D(physicsComponent.linearVelocity));

    reactphysics3d::Vector3 lockFactors(1,1,1);
    if (!physicsComponent.axisMotionAllowed[0]) { lockFactors[0] = 0; }
    if (!physicsComponent.axisMotionAllowed[1]) { lockFactors[1] = 0; }
    if (!physicsComponent.axisMotionAllowed[2]) { lockFactors[2] = 0; }
    pBody->setAngularLockAxisFactor(reactphysics3d::Vector3(lockFactors));

    pBody->setLinearDamping(physicsComponent.linearDamping);
    pBody->setAngularDamping(physicsComponent.angularDamping);

    //
    // Transform Properties
    //
    pBody->setTransform(GetRP3DTransform(transformComponent));
}

reactphysics3d::Transform Physics::GetRP3DTransform(const TransformComponent& transformComponent)
{
    return {
        ToRP3D(transformComponent.GetPosition()),
        ToRP3D(transformComponent.GetOrientation())
    };
}

std::expected<reactphysics3d::Collider*, bool> Physics::AddRigidCollider(reactphysics3d::RigidBody* pBody,
                                                                         const PhysicsComponent& physicsComponent,
                                                                         const BoundsComponent& boundsComponent,
                                                                         const TransformComponent& transformComponent)
{
    // Internal translation adjustment of the collider's models-space position. Is in addition to any
    // adjustment that was provided in boundsComponent & transformComponent
    glm::vec3 localPositionAdjustment(0,0,0);

    std::expected<reactphysics3d::CollisionShape*, bool> collisionShapeExpect;

    if (std::holds_alternative<Bounds_AABB>(boundsComponent.bounds))
    {
        collisionShapeExpect = CreateCollisionShapeAABB(boundsComponent, transformComponent, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_Sphere>(boundsComponent.bounds))
    {
        collisionShapeExpect = CreateCollisionShapeSphere(boundsComponent, transformComponent, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_Capsule>(boundsComponent.bounds))
    {
        collisionShapeExpect = CreateCollisionShapeCapsule(boundsComponent, transformComponent, localPositionAdjustment);
    }
    else if (std::holds_alternative<Bounds_HeightMap>(boundsComponent.bounds))
    {
        collisionShapeExpect = CreateCollisionShapeHeightMap(boundsComponent, transformComponent, localPositionAdjustment);
    }

    if (!collisionShapeExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "Physics::AddRigidCollider: Unsupported bounds type");
        return std::unexpected(false);
    }

    // Local transform
    reactphysics3d::Transform boundsLocalTransform = reactphysics3d::Transform::identity();
    boundsLocalTransform.setPosition(ToRP3D(boundsComponent.localTransform + localPositionAdjustment));
    boundsLocalTransform.setOrientation(ToRP3D(boundsComponent.localOrientation));

    auto pCollider = pBody->addCollider(*collisionShapeExpect, boundsLocalTransform);
    pCollider->getMaterial().setFrictionCoefficient(physicsComponent.frictionCoefficient);

    return pCollider;
}

std::expected<reactphysics3d::CollisionShape*, bool> Physics::CreateCollisionShapeAABB(
    const BoundsComponent& boundsComponent,
    const TransformComponent& transformComponent,
    glm::vec3&)
{
    const auto boundsAABB = std::get<Bounds_AABB>(boundsComponent.bounds);

    auto boxSize = boundsAABB.max - boundsAABB.min;
    boxSize *= transformComponent.GetScale();

    const auto boxSizeHalfExtents = boxSize / 2.0f;  // createBoxShape requires "half extents"

    return m_physicsCommon.createBoxShape(ToRP3D(boxSizeHalfExtents));
}

std::expected<reactphysics3d::CollisionShape*, bool> Physics::CreateCollisionShapeSphere(
    const BoundsComponent& boundsComponent,
    const TransformComponent& transformComponent,
    glm::vec3&)
{
    const auto boundsSphere = std::get<Bounds_Sphere>(boundsComponent.bounds);

    const auto transformScale = transformComponent.GetScale();
    const bool scaleIsUniform = transformScale.x == transformScale.y && transformScale.x == transformScale.z;

    if (!Common::Assert(scaleIsUniform, m_logger,
        "CreateSphereCollisionShape: Entity has a non-uniform scale applied"))
    {
        return std::unexpected(false);
    }

    const float radiusScaled = boundsSphere.radius * transformScale.x;

    return m_physicsCommon.createSphereShape(radiusScaled);
}

std::expected<reactphysics3d::CollisionShape*, bool> Physics::CreateCollisionShapeCapsule(
    const BoundsComponent& boundsComponent,
    const TransformComponent& transformComponent,
    glm::vec3&)
{
    const auto boundsCapsule = std::get<Bounds_Capsule>(boundsComponent.bounds);

    const auto transformScale = transformComponent.GetScale();
    const bool horizScaleIsUniform = transformScale.x == transformScale.z;

    if (!Common::Assert(horizScaleIsUniform, m_logger,
        "CreateCollisionShapeCapsule: Entity has non-uniform x/z scale applied"))
    {
        return std::unexpected(false);
    }

    const float radiusScaled = boundsCapsule.radius * transformScale.x;
    const float heightScaled = boundsCapsule.height * transformScale.y;

    return m_physicsCommon.createCapsuleShape(radiusScaled, heightScaled);
}

std::expected<reactphysics3d::CollisionShape*, bool> Physics::CreateCollisionShapeHeightMap(
    const BoundsComponent& boundsComponent,
    const TransformComponent& transformComponent,
    glm::vec3& localPositionAdjustment)
{
    const auto boundsHeightMap = std::get<Bounds_HeightMap>(boundsComponent.bounds);

    const auto heightMapDataOpt = m_worldResources->GetHeightMapData(boundsHeightMap.heightMapMeshId);
    if (!heightMapDataOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateCollisionShapeHeightMap: No such height map mesh found, id: {}", boundsHeightMap.heightMapMeshId.id);
        return std::unexpected(false);
    }

    const auto& heightMapData = *heightMapDataOpt;

    // How to scale the height map data points to mesh model coordinates
    const glm::vec3 scaleToMeshSize(
        (float)heightMapData->meshSize_worldSpace.w / ((float)heightMapData->dataSize.w - 1),
        1.0f,
        (float)heightMapData->meshSize_worldSpace.h / ((float)heightMapData->dataSize.h - 1)
    );

    // Scale the data points to model points, then scale by the model's object scale
    const glm::vec3 colliderScale = transformComponent.GetScale() * scaleToMeshSize;

    auto pCollisionShape = m_physicsCommon.createHeightFieldShape(
        (int)heightMapData->dataSize.w,
        (int)heightMapData->dataSize.h,
        (float)heightMapData->minValue,
        (float)heightMapData->maxValue,
        heightMapData->data.data(),
        reactphysics3d::HeightFieldShape::HeightDataType::HEIGHT_DOUBLE_TYPE,
        1,
        1.0f,
        reactphysics3d::Vector3(colliderScale.x, colliderScale.y, colliderScale.z)
    );

    // Adjust the height map upwards by half its height to undo the vertical centering rp3d does for height maps,
    // and also adjust it upwards by the minValue so that the 0 point of the height map is at the origin, not minScale.
    const double verticalOffset =
        ((heightMapData->maxValue - heightMapData->minValue) / 2.0f)
        + heightMapData->minValue;

    localPositionAdjustment = transformComponent.GetScale() * glm::vec3(0.0f, verticalOffset, 0.0f);

    return pCollisionShape;
}

void Physics::DestroyRigidBody(const EntityId& eid)
{
    const auto it = m_entityToRigidBody.find(eid);
    if (it == m_entityToRigidBody.cend())
    {
        return;
    }

    const auto it2 = m_bodyToEntity.find(it->second.pBody);
    if (it2 == m_bodyToEntity.cend())
    {
        return;
    }

    m_pPhysicsWorld->destroyRigidBody(it->second.pBody);
    m_entityToRigidBody.erase(it);
    m_bodyToEntity.erase(it2);
    SyncMetrics();
}

void Physics::ClearAll()
{
    DestroyAll();
    CreateWorld();
}

void Physics::CreateWorld()
{
    reactphysics3d::PhysicsWorld::WorldSettings worldSettings{};

    m_pPhysicsWorld = m_physicsCommon.createPhysicsWorld(worldSettings);

    SyncMetrics();
}

void Physics::DestroyAll()
{
    if (m_pPhysicsWorld != nullptr)
    {
        m_physicsCommon.destroyPhysicsWorld(m_pPhysicsWorld);
        m_pPhysicsWorld = nullptr;
    }
    m_entityToRigidBody.clear();
    m_bodyToEntity.clear();

    SyncMetrics();
}

void Physics::SyncMetrics()
{
    m_metrics->SetCounterValue(Engine_Physics_Rigid_Bodies_Count, m_entityToRigidBody.size());
}

bool Physics::ApplyRigidBodyLocalForce(const EntityId& eid, const glm::vec3& force)
{
    const auto bodyIt = m_entityToRigidBody.find(eid);
    if (bodyIt == m_entityToRigidBody.cend())
    {
        return false;
    }

    bodyIt->second.pBody->applyLocalForceAtCenterOfMass(ToRP3D(force));

    return true;
}

struct AllHitsReceiver : public reactphysics3d::RaycastCallback
{
    explicit AllHitsReceiver(const std::unordered_map<rp3d::CollisionBody*, EntityId>& _bodyToEntity)
        : m_bodyToEntity(_bodyToEntity)
    { }

    reactphysics3d::decimal notifyRaycastHit(const reactphysics3d::RaycastInfo& raycastInfo) override
    {
        const auto it = m_bodyToEntity.find(raycastInfo.body);
        if (it == m_bodyToEntity.cend())
        {
            // Ignore hits against geometry that somehow don't have an entity associated
            // with them (shouldn't ever be the case)
            return 1.0f;
        }

        hits.emplace_back(
            it->second,
            FromRP3D(raycastInfo.worldPoint),
            FromRP3D(raycastInfo.worldNormal)
        );

        return 1.0f;
    }

    std::vector<RaycastResult> hits;

    private:

        const std::unordered_map<rp3d::CollisionBody*, EntityId>& m_bodyToEntity;
};

std::vector<RaycastResult>
Physics::RaycastForCollisions(const glm::vec3& rayStart_worldSpace, const glm::vec3& rayEnd_worldSpace) const
{
    const auto ray = reactphysics3d::Ray(ToRP3D(rayStart_worldSpace), ToRP3D(rayEnd_worldSpace));

    AllHitsReceiver raycastReceiver(m_bodyToEntity);
    m_pPhysicsWorld->raycast(ray, &raycastReceiver);

    auto hits = raycastReceiver.hits;

    // Sort the hits by distance from the start point
    std::ranges::sort(hits, [&](const auto& l, const auto& r){
        const auto distanceToLeft = glm::distance(rayStart_worldSpace, l.hitPoint_worldSpace);
        const auto distanceToRight = glm::distance(rayStart_worldSpace, r.hitPoint_worldSpace);

        return distanceToLeft < distanceToRight;
    });

    return hits;
}

void Physics::EnableDebugRenderOutput(bool enable)
{
    m_pPhysicsWorld->setIsDebugRenderingEnabled(enable);

    if (enable)
    {
        auto& debugRenderer = m_pPhysicsWorld->getDebugRenderer();
        debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
    }

    m_debugRendering = enable;
}

std::vector<Render::Triangle> Physics::GetDebugTriangles() const
{
    if (!m_debugRendering) { return {}; }

    const auto& rp3dDebugTrianglesArray = m_pPhysicsWorld->getDebugRenderer().getTriangles();

    std::vector<Render::Triangle> debugTriangles;
    debugTriangles.reserve(rp3dDebugTrianglesArray.size());

    for (const auto& rp3dTriangle : rp3dDebugTrianglesArray)
    {
        debugTriangles.emplace_back(
            FromRP3D(rp3dTriangle.point1),
            FromRP3D(rp3dTriangle.point2),
            FromRP3D(rp3dTriangle.point3)
        );
    }

    return debugTriangles;
}

}
