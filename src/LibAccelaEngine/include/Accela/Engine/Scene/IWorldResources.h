/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDRESOURCES_H

#include <Accela/Engine/Scene/ITextureResources.h>
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

            //
            // Textures (split into its own subsystem)
            //

            [[nodiscard]] virtual ITextureResources::Ptr Textures() const = 0;

            //
            // Meshes
            //

            /**
             * Register a manually specified mesh's data.
             *
             * @param vertices The mesh's vertices
             * @param indices The mesh's indices
             * @param usage The mesh's usage pattern
             * @param tag A debug tag to associate with the mesh
             *
             * @return The MeshId associated with the registered Mesh, or Render::INVALID_ID on error
             */
            [[nodiscard]] virtual Render::MeshId RegisterStaticMesh(std::vector<Render::MeshVertex> vertices,
                                                                    std::vector<uint32_t> indices,
                                                                    Render::MeshUsage usage,
                                                                    const std::string& tag) = 0;

            /**
             * Generates a mesh from a provided (previously loaded) height map texture.
             *
             * Creates a mesh of heightMapDataSize.w x heightMapDataSize.h data points, read from the height map texture,
             * and creates a mesh that's meshSize_worldSpace size, containing a vertex for each data point.
             *
             * @param heightMapTextureId The TextureId of the height map to use
             * @param heightMapDataSize The dimensions of the height map data to create
             * @param meshSize_worldSpace The world space size of the produced mesh
             * @param displacementFactor Height scaling factor multiplied against each data point
             * @param usage The usage of the provided mesh
             * @param tag Debug tag to associate with the mesh
             *
             * @return The MeshId of the generated mesh, or INVALID_ID on error
             */
            [[nodiscard]] virtual Render::MeshId GenerateHeightMapMesh(const Render::TextureId& heightMapTextureId,
                                                                       const Render::USize& heightMapDataSize,
                                                                       const Render::USize& meshSize_worldSpace,
                                                                       const float& displacementFactor,
                                                                       Render::MeshUsage usage,
                                                                       const std::string& tag) = 0;

            /**
             * Destroy a previously loaded mesh
             *
             * @param meshId The MeshId of the mesh to be destroyed
             */
            virtual void DestroyMesh(Render::MeshId meshId) = 0;

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
