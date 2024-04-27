/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelAnimatorSystem.h"

#include "../Scene/ModelResources.h"

namespace Accela::Engine
{

ModelAnimatorSystem::ModelAnimatorSystem(Common::ILogger::Ptr logger, IWorldResources::Ptr worldResources)
    : m_logger(std::move(logger))
    , m_worldResources(std::move(worldResources))
{

}

void ModelAnimatorSystem::Execute(const RunState::Ptr& runState, entt::registry& registry)
{
    registry.view<RenderableStateComponent, ModelRenderableComponent>()
        .each([&](const auto&, auto& renderableComponent, auto &modelComponent)
          {
              // If the model component has no animation state, no work to do for it
              if (!modelComponent.animationState.has_value())
              {
                  return;
              }

              ProcessRenderableModelEntity(runState, renderableComponent, modelComponent);
          });
}

void ModelAnimatorSystem::ProcessRenderableModelEntity(const RunState::Ptr& runState,
                                                       RenderableStateComponent& renderableComponent,
                                                       ModelRenderableComponent& modelComponent)
{
    //
    // Calculate new animation time/state
    //
    const auto registeredModelOpt = std::dynamic_pointer_cast<ModelResources>(m_worldResources->Models())
        ->GetLoadedModel(modelComponent.modelName);
    if (!registeredModelOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ModelAnimatorSystem: Model doesn't exist: {}", modelComponent.modelName);
        return;
    }

    const auto modelAnimationIt = registeredModelOpt->model->animations.find(modelComponent.animationState->animationName);
    if (modelAnimationIt == registeredModelOpt->model->animations.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "ModelAnimatorSystem: Model doesn't contain animation: {}", modelComponent.animationState->animationName);
        return;
    }

    const double ticksDelta = modelAnimationIt->second.animationTicksPerSecond * ((float)runState->timeStep / 1000.0f);
    const double newAnimationTime = modelComponent.animationState->animationTime + ticksDelta;
    const bool animationReachedEnd = newAnimationTime >= modelAnimationIt->second.animationDurationTicks;

    // If the animation is at the end, and it's a one-time reset animation, then clear out animation state,
    // resetting the model back to its non-animated state
    if (animationReachedEnd && modelComponent.animationState->animationType == ModelAnimationType::OneTime_Reset)
    {
        modelComponent.animationState = std::nullopt;
    }
    // Otherwise, if the animation is at the end, and it's a one-time remain animation, the model should be kept in
    // its final tick of animation and the animation state kept around to keep it there
    else if (animationReachedEnd && modelComponent.animationState->animationType == ModelAnimationType::OneTime_Remain)
    {
        modelComponent.animationState->animationTime = modelAnimationIt->second.animationDurationTicks - 1;
    }
    // Otherwise, move the animation time forwards, looping it back to the beginning when needed
    else
    {
        modelComponent.animationState->animationTime =
            fmod(newAnimationTime, modelAnimationIt->second.animationDurationTicks);
    }

    //
    // Mark the renderable as dirty so it's updated
    //
    renderableComponent.state = ComponentState::Dirty;
}

}
