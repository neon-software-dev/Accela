#ifndef LIBACCELAENGINE_SRC_SCENE_IWORLDSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_IWORLDSYSTEM_H

#include "../RunState.h"

#include <entt/entt.hpp>

#include <memory>

namespace Accela::Engine
{
    class IWorldSystem
    {
        public:

            enum class Type
            {
                RendererSync,
                PhysicsSync,
                Audio,
                ModelAnimator
            };

            using Ptr = std::shared_ptr<IWorldSystem>;

        public:

            virtual ~IWorldSystem() = default;

            [[nodiscard]] virtual Type GetType() const noexcept = 0;

            virtual void Execute(const RunState::Ptr& runState, entt::registry& registry) = 0;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_IWORLDSYSTEM_H
