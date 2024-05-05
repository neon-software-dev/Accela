/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Physics/KinematicPlayerController.h>

namespace Accela::Engine
{

static constexpr auto MIN_JUMP_DURATION = std::chrono::milliseconds(100);
static constexpr auto MAX_JUMP_DURATION = std::chrono::milliseconds(300);
static constexpr auto WALK_SPEED_MULTIPLIER = 0.1f;
static constexpr auto SPRINT_SPEED_MULTIPLIER = 0.15f;
static constexpr float JUMP_SPEED = 0.2f;
static constexpr float COAST_SPEED_CHANGE = 0.01f;

std::expected<KinematicPlayerController::UPtr, bool> KinematicPlayerController::Create(
    const std::shared_ptr<IEngineRuntime>& engine,
    const std::string& name,
    const glm::vec3& position,
    const float& radius,
    const float& height
)
{
    PhysicsMaterial playerMaterial{};

    if (!engine->GetWorldState()->GetPhysics()->CreatePlayerController(
        name,
        position,
        radius,
        height,
        playerMaterial
    ))
    {
        return std::unexpected(false);
    }

    return std::make_unique<KinematicPlayerController>(Tag{}, engine, name);
}

KinematicPlayerController::KinematicPlayerController(Tag, std::shared_ptr<IEngineRuntime> engine, std::string name)
    : m_engine(std::move(engine))
    , m_name(std::move(name))
{

}

KinematicPlayerController::~KinematicPlayerController()
{
    DestroyInternal();
}

void KinematicPlayerController::DestroyInternal()
{
    m_engine->GetWorldState()->GetPhysics()->DestroyPlayerController(m_name);
}

glm::vec3 KinematicPlayerController::GetPosition() const
{
    const auto positionOpt = m_engine->GetWorldState()->GetPhysics()->GetPlayerControllerPosition(m_name);

    assert(positionOpt.has_value());

    if (!positionOpt)
    {
        return {0, 0, 0};
    }

    return *positionOpt;
}

void KinematicPlayerController::OnSimulationStep(const PlayerMovement& commandedMovement, const glm::vec3& lookUnit)
{
    const auto playerControllerState = m_engine->GetWorldState()->GetPhysics()->GetPlayerControllerState(m_name);
    if (!playerControllerState)
    {
        m_engine->GetLogger()->Log(Common::LogLevel::Error,
           "KinematicPlayerController::OnSimulationStep: PlayerControllerState doesn't exist");
        return;
    }

    //
    // Update State
    //
    m_locationState = CalculateLocationState(*playerControllerState);
    m_jumpState = CalculateJumpState(*playerControllerState, m_jumpState, commandedMovement.up);

    //
    // Calculate player manipulations
    //
    const auto commandedTranslation = CalculatePlayerVelocity(commandedMovement, lookUnit);

    //
    // Apply player manipulations
    //
    const float minDimension = std::min({commandedTranslation.x, commandedTranslation.y, commandedTranslation.z});
    const float minMoveDistance = minDimension / 10.0f;

    if (!m_engine->GetWorldState()->GetPhysics()->SetPlayerControllerMovement(
        m_name,
        commandedTranslation,
        minMoveDistance))
    {
        m_engine->GetLogger()->Log(Common::LogLevel::Error,
        "KinematicPlayerController::OnSimulationStep: Failed to update player movement");
    }
}

KinematicPlayerController::LocationState KinematicPlayerController::CalculateLocationState(const PlayerControllerState& playerControllerState)
{
    if (playerControllerState.collisionBelow)
    {
        return LocationState::Ground;
    }
    else
    {
        return LocationState::Air;
    }
}

std::optional<KinematicPlayerController::JumpState> KinematicPlayerController::CalculateJumpState(
    const PlayerControllerState& playerControllerState,
    const std::optional<JumpState>& previousJumpState,
    bool jumpCommanded)
{
    // If the user isn't commanding a jump, and we're not in a jump, nothing to do
    if (!jumpCommanded && !previousJumpState)
    {
        return std::nullopt;
    }

    // If the user commanded a jump, and we're not in a jump, try to start a new jump
    if (jumpCommanded && !previousJumpState)
    {
        const bool newJumpAllowed = playerControllerState.collisionBelow;

        // If we're not in a state where a new jump is possible, nothing to do
        if (!newJumpAllowed)
        {
            return std::nullopt;
        }

        // Start a new jump
        return JumpState{};
    }

    // At this point we're in a jump, but jumpCommanded may be true or false
    JumpState jumpState = *previousJumpState;

    switch (previousJumpState->state)
    {
        case JumpState::State::Jumping:
        {
            const auto jumpDuration = std::chrono::high_resolution_clock::now() - jumpState.jumpStartTime;
            const bool atMinJumpDuration = jumpDuration >= MIN_JUMP_DURATION;
            const bool atMaxJumpDuration = jumpDuration >= MAX_JUMP_DURATION;

            // If we're at the min jump duration and the user doesn't want to keep jumping, or if we've hit
            // the max jump duration, no matter what the user wants, transition to coasting state
            if ((!jumpCommanded && atMinJumpDuration) || atMaxJumpDuration)
            {
                jumpState.state = JumpState::State::Coasting;
            }

            // If we've hit something above us, transition to coasting state
            if (playerControllerState.collisionAbove)
            {
                jumpState.state = JumpState::State::Coasting;
            }

            jumpState.jumpSpeed = JUMP_SPEED;
        }
        break;
        case JumpState::State::Coasting:
        {
            // While coasting, incrementally decrease our velocity until there's no more upwards jump velocity left
            if (jumpState.jumpSpeed >= COAST_SPEED_CHANGE)
            {
                jumpState.jumpSpeed = std::max(0.0f, jumpState.jumpSpeed - COAST_SPEED_CHANGE);
            }

            if (jumpState.jumpSpeed <= COAST_SPEED_CHANGE)
            {
                jumpState.state = JumpState::State::FreeFall;
            }
        }
        break;
        case JumpState::State::FreeFall:
        {
            const auto onObject = playerControllerState.collisionBelow;

            // Reset our jump state to default when we land on an object
            if (onObject)
            {
                return std::nullopt;
            }
        }
        break;
    }

    return jumpState;
}

glm::vec3 KinematicPlayerController::CalculatePlayerVelocity(const PlayerMovement& commandedMovement,
                                                             const glm::vec3& lookUnit) const
{
    glm::vec3 commandedTranslation{0, 0, 0};

    // Apply movement commands from the user to the player
    std::optional<glm::vec3> normalizedXZMovement = GetNormalizedXZVector(commandedMovement);
    if (normalizedXZMovement)
    {
        const glm::vec3 xzPlaneForwardUnit = glm::normalize(glm::vec3(lookUnit.x, 0.0f, lookUnit.z));
        const auto upAndRightUnits = GetUpAndRightUnitsFrom(xzPlaneForwardUnit);

        // Determine movement in x,z directions relative to the forward unit
        const glm::vec3 xTranslation = upAndRightUnits.second * normalizedXZMovement->x;
        const glm::vec3 zTranslation = xzPlaneForwardUnit * normalizedXZMovement->z * -1.0f;
        const auto xzTranslationUnit = glm::normalize(xTranslation + zTranslation);

        float translationMultiplier = WALK_SPEED_MULTIPLIER;

        if (commandedMovement.sprint)
        {
            translationMultiplier = SPRINT_SPEED_MULTIPLIER;
        }

        commandedTranslation.x = xzTranslationUnit.x * translationMultiplier;
        commandedTranslation.z = xzTranslationUnit.z * translationMultiplier;
    }

    // Apply any active jump velocity to the player
    if (m_jumpState)
    {
        commandedTranslation.y += m_jumpState->jumpSpeed;
    }

    // Apply gravity to the player
    commandedTranslation.y += -0.1f;

    return commandedTranslation;
}

}
