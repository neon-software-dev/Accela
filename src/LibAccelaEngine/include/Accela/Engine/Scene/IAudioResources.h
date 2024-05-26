#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Common/AudioData.h>

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
             * Load an audio resource from a package
             *
             * @param resource Identifies the audio resource
             *
             * @return A future containing whether the operation was successful
             */
            [[nodiscard]] virtual std::future<bool> LoadAudio(const PackageResourceIdentifier& resource) = 0;

            /**
             * Loads a custom audio resource
             *
             * @param resource Identifies the audio resource
             * @param audioData The custom audio data to be loaded
             *
             * @return Whether the operation was successful
             */
            [[nodiscard]] virtual bool LoadAudio(const CustomResourceIdentifier& resource, const Common::AudioData::Ptr& audioData) = 0;

            /**
             * Load all audio resources from the specified package
             *
             * @param packageName The name of the package to load audio from
             *
             * @return A future containing whether all package audio loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllAudio(const PackageName& packageName) = 0;

            /**
             * Load all audio resources across all registered packages
             *
             * @return A future containing whether all audio loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllAudio() = 0;

            /**
             * Destroys a previously loaded audio resource
             *
             * @param resource Identifies the resource to be destroyed
             */
            virtual void DestroyAudio(const ResourceIdentifier& resource) = 0;

            /**
             * Destroys all previously loaded audio resources for a specific package
             *
             * @param packageName The name identifying the package
             */
            virtual void DestroyAllAudio(const PackageName& packageName) = 0;

            /**
             * Destroy all previously loaded audio resources
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IAUDIORESOURCES_H
