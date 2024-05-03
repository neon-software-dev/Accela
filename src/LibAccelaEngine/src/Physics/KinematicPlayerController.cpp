/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Physics/KinematicPlayerController.h>

namespace Accela::Engine
{

std::expected<KinematicPlayerController::UPtr, bool> KinematicPlayerController::Create(
    const std::shared_ptr<IEngineRuntime>& engine,
    const std::string& name,
    const glm::vec3& position,
    const float& radius,
    const float& height
)
{
    if (!engine->GetWorldState()->GetPhysics()->CreatePlayerController(
        name,
        position,
        radius,
        height
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
        const auto xzTranslation = xTranslation + zTranslation;

        commandedTranslation.x = xzTranslation.x * 0.1f;
        commandedTranslation.z = xzTranslation.z * 0.1f;
    }

    ProcessJumpState(commandedMovement.up);

    // Apply any active jump velocity to the player
    if (m_jumpState)
    {
        commandedTranslation += m_jumpState->jumpVelocity;
    }

    // Apply gravity to the player
    commandedTranslation.y += -0.1f;

    ////

    const float minDimension = std::min({commandedTranslation.x, commandedTranslation.y, commandedTranslation.z});
    const float minMoveDistance = minDimension / 10.0f;

    if (!m_engine->GetWorldState()->GetPhysics()->SetPlayerControllerMovement(
        m_name,
        commandedTranslation,
        minMoveDistance))
    {
        assert(false);
        return;
    }
}

void KinematicPlayerController::ProcessJumpState(bool jumpCommanded)
{
    static const auto maxJumpDuration = std::chrono::milliseconds(1000);

    if (!m_jumpState)
    {
        if (!jumpCommanded) { return; }

        m_jumpState = JumpState{};
    }

    const auto now = std::chrono::high_resolution_clock::now();

    switch (m_jumpState->state)
    {
        case JumpState::State::Jumping:
        {
            const auto jumpDuration = now - m_jumpState->jumpStartTime;
            const auto jumpDurationMaxed = jumpDuration >= maxJumpDuration;

            // If the player is no longer commanding a jump, or if they hit the max duration
            // that they can command a jump, switch to Coasting state
            if (!jumpCommanded || jumpDurationMaxed)
            {
                m_jumpState->state = JumpState::State::Coasting;
                m_jumpState->coastStartTime = now;
                return;
            }
        }
        break;
        case JumpState::State::Coasting:
        {
            m_jumpState->jumpVelocity.y -= 0.01f;

            if (m_jumpState->jumpVelocity.y <= 0.0f)
            {
                m_jumpState = std::nullopt;
            }
        }
        break;
    }
}

}
