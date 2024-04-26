/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H

#include <Accela/Engine/Scene/ITextureResources.h>
#include <Accela/Engine/Scene/IMeshResources.h>
#include <Accela/Engine/Scene/TextRender.h>
#include <Accela/Engine/Model/Model.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <Accela/Common/AudioData.h>

#include <memory>
#include <string>

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

            /** Interface to texture resource management */
            [[nodiscard]] virtual ITextureResources::Ptr Textures() const = 0;

            /** Interface to mesh resource management */
            [[nodiscard]] virtual IMeshResources::Ptr Meshes() const = 0;

            //
            // Materials
            //

            /**
             * Register an object material
             *
             * @param properties The properties of the material
             * @param tag A debug tag to associate with the material
             *
             * @return A MaterialId associated with the registered material, or Render::INVALID_ID on error
             */
            [[nodiscard]] virtual Render::MaterialId RegisterObjectMaterial(const Render::ObjectMaterialProperties& properties,
                                                                            const std::string& tag) = 0;

            /**
             * Destroy a previously registered material
             *
             * @param materialId The MaterialId of the material to be destroyed
             */
            virtual void DestroyMaterial(Render::MaterialId materialId) = 0;

            //
            // Audio
            //

            /**
             * Registers audio data
             *
             * @param name Unique name to associate with the audio data
             * @param audioData The audio data
             *
             * @return Whether the audio was registered successfully
             */
            [[nodiscard]] virtual bool RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData) = 0;

            /**
             * Destroys previously loaded audio data
             *
             * @param name The name associated with the audio data to be destroyed
             */
            virtual void DestroyAudio(const std::string& name) = 0;

            //
            // Text
            //

            /**
             * Blocking call which loads a font from the assets fonts directory
             *
             * @param fontFileName The filename of the font to be loaded
             * @param fontSize The font size to be loaded
             *
             * @return Whether the font was loaded successfully
             */
            [[nodiscard]] virtual bool LoadFontBlocking(const std::string& fontFileName, uint8_t fontSize) = 0;

            /**
             * Same as LoadFontBlocking, except allows for a range of font sizes to be loaded in one call.
             *
             * @param fontFileName The filename of the font to be loaded
             * @param startFontSize The inclusive starting font size to load
             * @param endFontSize The inclusive ending font size to load
             *
             * @return Whether all the font sizes loaded successfully
             */
            [[nodiscard]] virtual bool LoadFontBlocking(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize) = 0;

            /**
             * @return Whether the specific font file and font size is currently loaded
             */
            [[nodiscard]] virtual bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) = 0;

            //
            // Models
            //

            /**
             * Register a model
             *
             * @param modelName A unique name to associate with the model
             * @param model The model's data
             *
             * @return Whether the model was registered succesfully
             */
            [[nodiscard]] virtual bool RegisterModel(const std::string& modelName, const Model::Ptr& model) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H
