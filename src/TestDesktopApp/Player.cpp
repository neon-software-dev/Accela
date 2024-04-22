/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Player.h"

namespace Accela
{

static constexpr float PLAYER_HEIGHT = 1.8f; // m
static constexpr float PLAYER_RADIUS = 0.4f; // m
static constexpr float PLAYER_MASS = 70.0f; // kg
static constexpr float MAX_XZ_SPEED = 5.0f; // m/s
static constexpr float GRAV_PERCENT = 0.75f; // %
static constexpr float FRICTION_COEFFICIENT = 4.0f;
static constexpr float LINEAR_DAMPING = 0.4f;
static constexpr float GROUND_MOVE_FORCE = 6000.0f; // N
static constexpr float AIR_MOVE_FORCE = 200.0f; // N
static constexpr float JUMP_FORCE = 40000.0f; // N

static constexpr float HALF_PLAYER_HEIGHT = PLAYER_HEIGHT / 2.0f; // m
static constexpr float GRAV_FORCE = PLAYER_MASS * 9.80f;

Player::UPtr Player::Create(const Engine::IEngineRuntime::Ptr& engine,
                            const std::string& sceneName,
                            const Engine::SceneEvents::Ptr& sceneEvents,
                            const glm::vec3& position,
                            const std::optional<Render::MeshId>& playerMeshId,
                            const std::optional<Render::MaterialId>& playerMaterialId)
{
    assert(PLAYER_HEIGHT >= 2.0f * PLAYER_RADIUS);

    auto eid = engine->GetWorldState()->CreateEntity();

    if (playerMeshId && playerMaterialId)
    {
        auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
        objectRenderableComponent.sceneName = sceneName;
        objectRenderableComponent.meshId = *playerMeshId;
        objectRenderableComponent.materialId = *playerMaterialId;

        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);
    }

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);

    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::DynamicBody(PLAYER_MASS);
    physicsComponent.axisMotionAllowed = {false, false, false};
    physicsComponent.frictionCoefficient = FRICTION_COEFFICIENT;
    physicsComponent.linearDamping = LINEAR_DAMPING;

    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);

    const float distanceBetweenCapsuleSpheres = PLAYER_HEIGHT - (2.0f * PLAYER_RADIUS);

    auto boundsComponent = Engine::BoundsComponent(
        Engine::Bounds_Capsule(PLAYER_RADIUS, distanceBetweenCapsuleSpheres)
    );

    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, boundsComponent);

    return std::make_unique<Player>(Tag{}, engine, sceneName, sceneEvents, eid);
}

Player::Player(Tag, Engine::IEngineRuntime::Ptr engine,
               std::string sceneName,
               Engine::SceneEvents::Ptr sceneEvents,
               const Engine::EntityId& eid)
    : Engine::SceneEntity(std::move(engine), std::move(sceneName), std::move(sceneEvents))
    , m_eid(eid)
{

}

Player::~Player()
{
    DestroyInternal();
}

void Player::Destroy()
{
    DestroyInternal();
}

void Player::DestroyInternal()
{
    m_engine->GetWorldState()->DestroyEntity(m_eid);
}

glm::vec3 Player::GetPosition() const noexcept
{
    const auto transformComponent = GetTransformComponent();
    if (!transformComponent)
    {
        return glm::vec3(0);
    }

    return transformComponent->GetPosition();
}

glm::vec3 Player::GetEyesPosition() const noexcept
{
    return GetPosition() + glm::vec3(0, HALF_PLAYER_HEIGHT / 2.0f, 0);
}

inline glm::vec3 ClosestPointOnOriginPlane(const glm::vec3& v, const glm::vec3& normal)
{
    return v + ((-glm::dot(v, normal)) * normal);
}

void Player::OnMovementCommanded(const glm::vec3& xzInput, const glm::vec3& lookUnit)
{
    SyncCurrentState();

    glm::vec3 movementUnit = MapMovementToLookPlane(xzInput, lookUnit);

    // If we're touching the ground, project the requested movement onto the ground's slope's plane
    if (IsTouchingGround())
    {
        movementUnit = glm::normalize(
            ClosestPointOnOriginPlane(movementUnit, m_groundRaycast->raycast.hitNormal_worldSpace)
        );
    }
    else
    {
        movementUnit = glm::normalize(
            ClosestPointOnOriginPlane(movementUnit, {0,1,0})
        );
    }

    // Apply a movement force
    float moveForceMagnitude = GROUND_MOVE_FORCE;

    if (m_locationState == LocationState::Air)
    {
        moveForceMagnitude = AIR_MOVE_FORCE;
    }

    const auto movementForce = movementUnit * moveForceMagnitude;

    m_engine->GetWorldState()->GetPhysics()->ApplyRigidBodyLocalForce(m_eid, movementForce);
}

glm::vec3 Player::MapMovementToLookPlane(const glm::vec3& xzInput,
                                         const glm::vec3& lookUnit)
{
    const glm::vec3 forwardUnit = glm::vec3(lookUnit.x, 0.0f, lookUnit.z);

    const auto upAndRight = GetUpAndRightFrom(forwardUnit);

    // Determine movement in x,z directions relative to the forward unit
    const glm::vec3 xTranslation = upAndRight.second * xzInput.x;
    const glm::vec3 zTranslation = forwardUnit * xzInput.z * -1.0f;

    // Unit combined x/z direction we were commanded to move towards
    auto unitTranslation = glm::normalize(xTranslation + zTranslation);

    return unitTranslation;
}

void Player::OnJumpCommanded()
{
    SyncCurrentState();

    // Can only jump if we're touching ground
    if (!IsTouchingGround())
    {
        return;
    }

    auto physicsComponent = GetPhysicsComponent();
    if (!physicsComponent) { return; }

    // Zero out any existing vertical velocity
    physicsComponent->linearVelocity = glm::vec3(physicsComponent->linearVelocity.x, 0, physicsComponent->linearVelocity.z);
    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), m_eid, *physicsComponent);

    // Apply a jump force
    const auto jumpForce = m_groundRaycast->raycast.hitNormal_worldSpace * JUMP_FORCE;
    m_engine->GetWorldState()->GetPhysics()->ApplyRigidBodyLocalForce(m_eid, jumpForce);
}

std::optional<Engine::TransformComponent> Player::GetTransformComponent() const
{
    return Engine::GetComponent<Engine::TransformComponent>(m_engine->GetWorldState(), m_eid);
}

std::optional<Engine::PhysicsComponent> Player::GetPhysicsComponent() const
{
    return Engine::GetComponent<Engine::PhysicsComponent>(m_engine->GetWorldState(), m_eid);
}

inline bool AreUnitVectorsParallel(const glm::vec3& a, const glm::vec3& b)
{
    return glm::abs(glm::dot(a, b)) > .9999f;
}

std::pair<glm::vec3, glm::vec3> Player::GetUpAndRightFrom(const glm::vec3& lookUnit)
{
    auto upUnit = glm::vec3(0, 1, 0);

    // crossing vectors is undefined if they're parallel, so choose an alternate up
    // vector in those cases
    if (AreUnitVectorsParallel(lookUnit, upUnit))
    {
        // If looking up, then our "up" is re-adjusted to be pointing out of the screen
        if (lookUnit.y >= 0.0f)  { upUnit = glm::vec3(0,0,1); }
        // If looking down, then our "up" is re-adjusted to be pointing into the screen
        else                     { upUnit = glm::vec3(0,0,-1); }
    }

    const auto rightUnit = glm::normalize(glm::cross(lookUnit, upUnit));
    upUnit = glm::normalize(glm::cross(rightUnit, lookUnit));

    return std::make_pair(upUnit, rightUnit);
}

void Player::OnSimulationStep(const Engine::IEngineRuntime::Ptr& engine, unsigned int timeStep)
{
    SceneCallbacks::OnSimulationStep(engine, timeStep);

    SyncCurrentState();

    ApplyAntiGrav(GRAV_PERCENT);
    EnforceSpeedLimit();

    UpdateMetrics();
}

void Player::SyncCurrentState()
{
    const auto transformComponent = GetTransformComponent();
    if (!transformComponent) { return; }

    m_playerBounds = GetPlayerBounds(transformComponent->GetPosition());
    m_groundRaycast = RaycastForGround(m_playerBounds, 4);
    m_locationState = DetermineLocationState(m_groundRaycast);
}

void Player::ApplyAntiGrav(float gravPercent)
{
    assert(gravPercent >= 0.0f);
    assert(gravPercent <= 1.0f);

    if (gravPercent == 1.0f) { return; }

    // Apply an upwards force to counteract normal gravity
    const auto antiGravForceMag = GRAV_FORCE * (1 - GRAV_PERCENT);
    const auto antiGravForce = glm::vec3(0,antiGravForceMag,0);
    m_engine->GetWorldState()->GetPhysics()->ApplyRigidBodyLocalForce(m_eid, antiGravForce);
}

void Player::EnforceSpeedLimit()
{
    auto physicsComponent = GetPhysicsComponent();
    if (!physicsComponent) { return; }

    const auto playerLinearVelocity = physicsComponent->linearVelocity;
    const auto playerXZVelocity = glm::vec3(playerLinearVelocity.x, 0.0f, playerLinearVelocity.z);

    if (glm::length(playerXZVelocity) > MAX_XZ_SPEED)
    {
        // Reduce the xz velocity to MAX_XZ_SPEED, in the same direction
        physicsComponent->linearVelocity = glm::normalize(playerXZVelocity) * MAX_XZ_SPEED;

        // Restore the player vertical velocity, as we don't apply a speed limit to that
        physicsComponent->linearVelocity.y = playerLinearVelocity.y;

        Engine::AddOrUpdateComponent(m_engine->GetWorldState(), m_eid, *physicsComponent);
    }
}

void Player::UpdateMetrics()
{
    if (m_groundRaycast)
    {
        m_engine->GetMetrics()->SetDoubleValue("PLAYER_ABOVE_GROUND", m_groundRaycast->distanceFromCapsule);
    }
    else
    {
        m_engine->GetMetrics()->SetDoubleValue("PLAYER_ABOVE_GROUND", -1);
    }

    switch (m_locationState)
    {
        case LocationState::Ground:
            m_engine->GetMetrics()->SetCounterValue("PLAYER_STATE", 0);
        break;
        case LocationState::Slope:
            m_engine->GetMetrics()->SetCounterValue("PLAYER_STATE", 1);
        break;
        case LocationState::Air:
            m_engine->GetMetrics()->SetCounterValue("PLAYER_STATE", 2);
        break;
    }
}

bool Player::IsTouchingGround() const
{
    return m_locationState == LocationState::Ground || m_locationState == LocationState::Slope;
}

Player::PlayerBounds Player::GetPlayerBounds(const glm::vec3& playerPosition)
{
    return {
        playerPosition,
        playerPosition + glm::vec3(0, HALF_PLAYER_HEIGHT, 0),
        playerPosition - glm::vec3(0, HALF_PLAYER_HEIGHT, 0)
    };
}

std::optional<Player::GroundRaycast> Player::FindGroundByRayOffset(const PlayerBounds& playerBounds, const glm::vec2& rayXZOffset) const
{
    const float topOffset = 0.01f;
    const float maxSearchDistance = PLAYER_RADIUS;

    const auto rayStart =
        playerBounds.topCenter +
        glm::vec3(rayXZOffset.x, topOffset, rayXZOffset.y);

    const auto rayEnd =
        playerBounds.bottomCenter +
        glm::vec3(rayXZOffset.x, -maxSearchDistance, rayXZOffset.y);

    const auto rayXZOffsetLength = glm::length(rayXZOffset);

    auto rayHits = m_engine->GetWorldState()->GetPhysics()->RaycastForCollisions(
        rayStart,
        rayEnd
    );

    // The first hit is the intersection of the ray with the top of the player
    // The second hit is the intersection of the ray with whatever is below the player
    if (rayHits.size() < 2)
    {
        return std::nullopt;
    }

    const auto groundHit = rayHits.at(1);

    const float capsuleBottomVertOffset =
        PLAYER_RADIUS -
        std::sqrt(std::pow(PLAYER_RADIUS, 2.0f) - std::pow(rayXZOffsetLength, 2.0f));

    const auto bottomOfCapsuleAlongRay =
        playerBounds.bottomCenter +
        glm::vec3(rayXZOffset.x, capsuleBottomVertOffset, rayXZOffset.y);

    auto groundDistance = glm::distance(
        bottomOfCapsuleAlongRay,
        groundHit.hitPoint_worldSpace
    );

    return GroundRaycast(groundDistance, groundHit);
}

std::optional<Player::GroundRaycast> Player::RaycastForGround(const PlayerBounds& playerBounds, unsigned int numPerimeterTestPoints) const
{
    std::optional<GroundRaycast> closestHit;

    const auto centerCastGroundResult = FindGroundByRayOffset(playerBounds, {0,0});
    if (centerCastGroundResult)
    {
        closestHit = centerCastGroundResult;
    }

    //
    // Ray cast points around the perimeter of the player capsule
    //

    // Don't test at PLAYER_RADIUS away from the center as it'd be raycasting directly down the side of the
    // capsule which seems to produce weird/random results, so test slightly in from that distance instead.
    const float perimeterRadius = PLAYER_RADIUS - 0.001f;

    const float perimeterAnglePerPoint = 360.0f / (float)numPerimeterTestPoints;
    float perimeterTestAngle = 0.0f;

    for (unsigned int x = 0; x < numPerimeterTestPoints; ++x)
    {
        const auto perimeterRayXZOffset = glm::vec2(
            perimeterRadius * std::cos(glm::radians(perimeterTestAngle)),
            perimeterRadius * std::sin(glm::radians(perimeterTestAngle))
        );

        const auto groundHit = FindGroundByRayOffset(playerBounds, perimeterRayXZOffset);
        if (groundHit)
        {
            if (!closestHit || (groundHit->distanceFromCapsule < closestHit->distanceFromCapsule))
            {
                closestHit = groundHit;
            }
        }

        perimeterTestAngle += perimeterAnglePerPoint;
    }

    return closestHit;
}

Player::LocationState Player::DetermineLocationState(const std::optional<GroundRaycast>& groundRaycast)
{
    // If we don't even see the ground underneath, we're in the air
    if (!groundRaycast)
    {
        return LocationState::Air;
    }

    // If we're too far away from the ground, we're in the air
    if (groundRaycast->distanceFromCapsule >= 0.2f)
    {
        return LocationState::Air;
    }

    // At this point we're close enough to the ground, just determine if we're on a slope or not
    const auto groundNormalUnit = glm::normalize(groundRaycast->raycast.hitNormal_worldSpace);

    // 1 == fully vertical, 0 == fully horizontal
    const auto groundYAxisDot = glm::dot(groundNormalUnit, glm::vec3(1,0,0));

    // Less than a 5-degree angle is considered flat
    const auto onFlatGround = std::abs(groundYAxisDot) <= 0.05f;

    if (onFlatGround)
    {
        return LocationState::Ground;
    }
    else
    {
        return LocationState::Slope;
    }
}

}
