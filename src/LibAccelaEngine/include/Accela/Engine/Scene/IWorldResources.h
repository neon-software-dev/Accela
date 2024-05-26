#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H

#include <Accela/Engine/Scene/IPackageResources.h>
#include <Accela/Engine/Scene/ITextureResources.h>
#include <Accela/Engine/Scene/IMeshResources.h>
#include <Accela/Engine/Scene/IMaterialResources.h>
#include <Accela/Engine/Scene/IAudioResources.h>
#include <Accela/Engine/Scene/IFontResources.h>
#include <Accela/Engine/Scene/IModelResources.h>
#include <Accela/Engine/Scene/TextRender.h>
#include <Accela/Engine/Model/Model.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <Accela/Common/AudioData.h>

#include <memory>
#include <string>
#include <future>

namespace Accela::Engine
{
    /**
     * Main user-facing interface to functionality for loading resources (textures / fonts / models / etc) into
     * the engine for future use.
     */
    class IWorldResources
    {
        public:

            using Ptr = std::shared_ptr<IWorldResources>;

        public:

            virtual ~IWorldResources() = default;

            /** Interface to package management */
            [[nodiscard]] virtual IPackageResources::Ptr Packages() const = 0;

            /** Interface to texture resource management */
            [[nodiscard]] virtual ITextureResources::Ptr Textures() const = 0;

            /** Interface to mesh resource management */
            [[nodiscard]] virtual IMeshResources::Ptr Meshes() const = 0;

            /** Interface to material resource management */
            [[nodiscard]] virtual IMaterialResources::Ptr Materials() const = 0;

            /** Interface to audio resource management */
            [[nodiscard]] virtual IAudioResources::Ptr Audio() const = 0;

            /** Interface to font resource management */
            [[nodiscard]] virtual IFontResources::Ptr Fonts() const = 0;

            /** Interface to model resource management */
            [[nodiscard]] virtual IModelResources::Ptr Models() const = 0;

            /**
             * Opens the specified package if it isn't opened, and loads all resources from it into
             * the resources subsystems and the renderer.
             *
             * Note: Each font resource will have sizes 8 through 20, inclusive, loaded. Any additional
             * sizes you may want to use requires loading those sizes via the IFontResources system.
             *
             * @param packageName The name of the package to be loaded
             * @param resultWhen When the result future should be signaled
             *
             * @return Whether the package was opened and all of its resources successfully loaded
             */
            [[nodiscard]] virtual std::future<bool> EnsurePackageResources(const PackageName& packageName,
                                                                           ResultWhen resultWhen) = 0;

            /**
            * Destroy all previously loaded resources across all resource systems
            */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H
