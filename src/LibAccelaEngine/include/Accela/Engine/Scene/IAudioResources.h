/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H

#include <Accela/Engine/Common.h>

#include <Accela/Common/AudioData.h>

#include <string>
#include <future>
#include <memory>

namespace Accela::Engine
{
    /**
     * Encapsulates audio resource operations
     */
    class IAudioResources
    {
        public:

            using Ptr = std::shared_ptr<IAudioResources>;

        public:

            virtual ~IAudioResources() = default;

            /**
             * Load an audio file from assets
             *
             * @param audioFileName The filename of the audio file
             *
             * @return A future that's signaled when the operation is completed
             */
            [[nodiscard]] virtual std::future<bool> LoadAssetsAudio(const std::string& audioFileName) = 0;

            /**
             * Load all audio files from assets
             *
             * @return A future that's signaled when the operation is completed
             */
            [[nodiscard]] virtual std::future<bool> LoadAllAssetAudio() = 0;

            /**
             * Loads audio data
             *
             * @param name Unique name to associate with the audio data
             * @param audioData The audio data
             *
             * @return Whether the audio was loaded successfully
             */
            [[nodiscard]] virtual bool LoadAudio(const std::string& name, const Common::AudioData::Ptr& audioData) = 0;

            /**
             * Destroys previously registered audio data
             *
             * @param name The name associated with the audio data to be destroyed
             */
            virtual void DestroyAudio(const std::string& name) = 0;

            /**
             * Destroy all previously loaded audio data
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H
