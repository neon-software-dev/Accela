#ifndef LIBACCELAENGINE_SRC_SCENE_AUDIOSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_AUDIOSYSTEM_H

#include "IWorldSystem.h"

#include "./ForwardDeclares.h"

#include "../Component/AudioComponent.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Audio/AudioListener.h>

#include <Accela/Common/Log/ILogger.h>

#include <entt/entt.hpp>

namespace Accela::Engine
{
    class AudioSystem : public IWorldSystem
    {
        public:

            AudioSystem(Common::ILogger::Ptr logger, AudioManagerPtr audioManager);

            [[nodiscard]] Type GetType() const noexcept override { return Type::Audio; };

            void Execute(const RunState::Ptr& runState, entt::registry& registry) override;

            void SetAudioListener(const AudioListener& listener);

        private:

            void UpdateAudioListener();

            void UpdateSourceProperties(AudioComponent& audioComponent,
                                        const TransformComponent& transformComponent) const;

            void StartAndStopAudio(entt::registry& registry,
                                   const EntityId& entity,
                                   AudioComponent& audioComponent);

        private:

            Common::ILogger::Ptr m_logger;
            AudioManagerPtr m_audioManager;

            AudioListener m_listener{};
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_AUDIOSYSTEM_H
