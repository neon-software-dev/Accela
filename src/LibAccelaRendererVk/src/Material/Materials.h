/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MATERIAL_MATERIALS_H
#define LIBACCELARENDERERVK_SRC_MATERIAL_MATERIALS_H

#include "IMaterials.h"
#include "RenderMaterial.h"

#include "../ForwardDeclares.h"
#include "../Util/ExecutionContext.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <unordered_map>
#include <unordered_set>

namespace Accela::Render
{
    class Materials : public IMaterials
    {
        public:

            Materials(Common::ILogger::Ptr logger,
                      VulkanObjsPtr vulkanObjs,
                      PostExecutionOpsPtr postExecutionOps,
                      Ids::Ptr ids,
                      ITexturesPtr textures,
                      IBuffersPtr buffers);

            bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                            VkQueue vkTransferQueue) override;
            void Destroy() override;

            [[nodiscard]] bool CreateMaterial(const Material::Ptr& material) override;
            [[nodiscard]] bool UpdateMaterial(const Material::Ptr& material) override;
            [[nodiscard]] std::optional<LoadedMaterial> GetLoadedMaterial(MaterialId materialId) const override;
            [[nodiscard]] std::optional<DataBufferPtr> GetMaterialBufferForType(const Material::Type& materialType) const override;
            void DestroyMaterial(MaterialId materialId, bool destroyImmediately) override;

        private:

            [[nodiscard]] std::expected<DataBufferPtr, bool> EnsureMaterialBuffer(const Material::Type& materialType);

            bool UpdateMaterial(const ExecutionContext& executionContext,
                                const LoadedMaterial& loadedMaterial,
                                const RenderMaterial& newMaterialData);

            void OnMaterialLoadFinished(const LoadedMaterial& loadedMaterial);
            void DestroyMaterialObjects(const LoadedMaterial& loadedMaterial);

            [[nodiscard]] static RenderMaterial ToRenderMaterial(const Material::Ptr& material);
            [[nodiscard]] static RenderMaterial ObjectMaterialToRenderMaterial(const ObjectMaterial::Ptr& material);

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;
            PostExecutionOpsPtr m_postExecutionOps;
            Ids::Ptr m_ids;
            ITexturesPtr m_textures;
            IBuffersPtr m_buffers;

            VulkanCommandPoolPtr m_transferCommandPool;
            VkQueue m_vkTransferQueue{VK_NULL_HANDLE};

            std::unordered_map<MaterialId, LoadedMaterial> m_materials;
            std::unordered_map<Material::Type, DataBufferPtr> m_materialBuffers;

            std::unordered_set<MaterialId> m_materialsLoading;
            std::unordered_set<MaterialId> m_materialsToDestroy;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MATERIAL_MATERIALS_H
