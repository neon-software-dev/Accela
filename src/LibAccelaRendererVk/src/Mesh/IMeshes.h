/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MESH_IMESHES_H
#define LIBACCELARENDERERVK_SRC_MESH_IMESHES_H

#include "LoadedMesh.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Mesh/Mesh.h>

#include <vulkan/vulkan.h>

#include <optional>

namespace Accela::Render
{
    class IMeshes
    {
        public:

            virtual ~IMeshes() = default;

            virtual bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                                    VkQueue vkTransferQueue) = 0;
            virtual void Destroy() = 0;

            [[nodiscard]] virtual bool LoadMesh(const Mesh::Ptr& mesh, MeshUsage usage) = 0;
            [[nodiscard]] virtual bool UpdateMesh(const Mesh::Ptr& mesh) = 0;
            [[nodiscard]] virtual std::optional<LoadedMesh> GetLoadedMesh(MeshId meshId) const = 0;
            virtual void DestroyMesh(MeshId meshId, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MESH_IMESHES_H
