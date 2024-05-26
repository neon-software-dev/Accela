#ifndef LIBACCELAENGINE_SRC_SCENE_MODELANIMATORSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_MODELANIMATORSYSTEM_H

#include "IWorldSystem.h"

#include "../Component/RenderableStateComponent.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Scene/IWorldResources.h>
#include <Accela/Engine/Component/ModelRenderableComponent.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Engine
{
    class ModelAnimatorSystem : public IWorldSystem
    {
        public:

            ModelAnimatorSystem(Common::ILogger::Ptr logger, IWorldResources::Ptr worldResources);

            [[nodiscard]] Type GetType() const noexcept override { return Type::ModelAnimator; };

            void Execute(const RunState::Ptr& runState, entt::registry& registry) override;

        private:

            void ProcessRenderableModelEntity(const RunState::Ptr& runState,
                                              RenderableStateComponent& renderableComponent,
                                              ModelRenderableComponent& modelComponent);

        private:

            Common::ILogger::Ptr m_logger;
            IWorldResources::Ptr m_worldResources;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_MODELANIMATORSYSTEM_H
