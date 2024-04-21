/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MATERIAL_IMATERIALS_H
#define LIBACCELARENDERERVK_SRC_MATERIAL_IMATERIALS_H

#include "LoadedMaterial.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Material/Material.h>

#include <vulkan/vulkan.h>

#include <optional>

namespace Accela::Render
{
    class IMaterials
    {
        public:

            virtual ~IMaterials() = default;

            virtual bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                                    VkQueue vkTransferQueue) = 0;
            virtual void Destroy() = 0;

            [[nodiscard]] virtual bool CreateMaterial(const Material::Ptr& material) = 0;
            [[nodiscard]] virtual bool UpdateMaterial(const Material::Ptr& material) = 0;
            [[nodiscard]] virtual std::optional<LoadedMaterial> GetLoadedMaterial(MaterialId materialId) const = 0;
            [[nodiscard]] virtual std::optional<DataBufferPtr> GetMaterialBufferForType(const Material::Type& materialType) const = 0;
            virtual void DestroyMaterial(MaterialId materialId, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MATERIAL_IMATERIALS_H
