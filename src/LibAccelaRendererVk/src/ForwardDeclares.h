/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FORWARDDECLARES_H
#define LIBACCELARENDERERVK_SRC_FORWARDDECLARES_H

#include <memory>

namespace Accela::Render
{
    class IVulkanCalls; using IVulkanCallsPtr = std::shared_ptr<IVulkanCalls>;
    class IVulkanContext; using IVulkanContextPtr = std::shared_ptr<IVulkanContext>;
    class IVMA; using IVMAPtr = std::shared_ptr<IVMA>;
    class VulkanObjs; using VulkanObjsPtr = std::shared_ptr<VulkanObjs>;
    class VulkanInstance; using VulkanInstancePtr = std::shared_ptr<VulkanInstance>;
    class VulkanSurface; using VulkanSurfacePtr = std::shared_ptr<VulkanSurface>;
    class VulkanPhysicalDevice; using VulkanPhysicalDevicePtr = std::shared_ptr<VulkanPhysicalDevice>;
    class VulkanDevice; using VulkanDevicePtr = std::shared_ptr<VulkanDevice>;
    class VulkanSwapChain; using VulkanSwapChainPtr = std::shared_ptr<VulkanSwapChain>;
    class VulkanShaderModule; using VulkanShaderModulePtr = std::shared_ptr<VulkanShaderModule>;
    class IShaders; using IShadersPtr = std::shared_ptr<IShaders>;
    class VulkanDescriptorSetLayout; using VulkanDescriptorSetLayoutPtr = std::shared_ptr<VulkanDescriptorSetLayout>;
    class VulkanDescriptorSet; using VulkanDescriptorSetPtr = std::shared_ptr<VulkanDescriptorSet>;
    class VulkanDescriptorPool; using VulkanDescriptorPoolPtr = std::shared_ptr<VulkanDescriptorPool>;
    class DescriptorSets; using DescriptorSetsPtr = std::shared_ptr<DescriptorSets>;
    class ProgramDef; using ProgramDefPtr = std::shared_ptr<ProgramDef>;
    class IPrograms; using IProgramsPtr = std::shared_ptr<IPrograms>;
    class VulkanRenderPass; using VulkanRenderPassPtr = std::shared_ptr<VulkanRenderPass>;
    class VulkanFramebuffer; using VulkanFramebufferPtr = std::shared_ptr<VulkanFramebuffer>;
    class VulkanPipeline; using VulkanPipelinePtr = std::shared_ptr<VulkanPipeline>;
    class IPipelineFactory; using IPipelineFactoryPtr = std::shared_ptr<IPipelineFactory>;
    class VulkanCommandBuffer; using VulkanCommandBufferPtr = std::shared_ptr<VulkanCommandBuffer>;
    class VulkanCommandPool; using VulkanCommandPoolPtr = std::shared_ptr<VulkanCommandPool>;
    class PostExecutionOps; using PostExecutionOpsPtr = std::shared_ptr<PostExecutionOps>;
    class ITextures; using ITexturesPtr = std::shared_ptr<ITextures>;
    class IBuffers; using IBuffersPtr = std::shared_ptr<IBuffers>;
    class Buffer; using BufferPtr = std::shared_ptr<Buffer>;
    class DataBuffer; using DataBufferPtr = std::shared_ptr<DataBuffer>;
    class IMeshes; using IMeshesPtr = std::shared_ptr<IMeshes>;
    class IFramebuffers; using IFramebuffersPtr = std::shared_ptr<IFramebuffers>;
    class IRenderables; using IRenderablesPtr = std::shared_ptr<IRenderables>;
    class IMaterials; using IMaterialsPtr = std::shared_ptr<IMaterials>;
    class ILights; using ILightsPtr = std::shared_ptr<ILights>;
}

#endif //LIBACCELARENDERERVK_SRC_FORWARDDECLARES_H
