/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_SYNCHRONIZATION_H
#define LIBACCELARENDERERVK_SRC_UTIL_SYNCHRONIZATION_H

#include "../ForwardDeclares.h"

#include "../Image/LoadedImage.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Accela::Render
{
    //
    // Semaphores
    //
    struct SemaphoreWait
    {
        SemaphoreWait(VkSemaphore _semaphore, VkPipelineStageFlags _stageFlags);

        VkSemaphore semaphore{VK_NULL_HANDLE};
        VkPipelineStageFlags stageFlags{0};
    };

    struct WaitOn
    {
        explicit WaitOn(const std::vector<SemaphoreWait>& _semaphores);

        static WaitOn None() { return WaitOn({}); }

        std::vector<VkSemaphore> semaphores;
        std::vector<VkPipelineStageFlags> stageFlags;
    };

    struct SignalOn
    {
        explicit SignalOn(std::vector<VkSemaphore> _semaphores);

        static SignalOn None() { return SignalOn({}); }

        std::vector<VkSemaphore> semaphores;
    };

    //
    // Pipeline Barriers
    //
    struct SourceStage
    {
        explicit SourceStage(const VkPipelineStageFlagBits& _stage)
            : stage(_stage)
        { }

        VkPipelineStageFlagBits stage;
    };

    struct DestStage
    {
        explicit DestStage(const VkPipelineStageFlagBits& _stage)
            : stage(_stage)
        { }

        VkPipelineStageFlagBits stage;
    };

    struct SourceAccess
    {
        explicit SourceAccess(const VkAccessFlags& _flags)
            : flags(_flags)
        { }

        VkAccessFlags flags;
    };

    struct DestAccess
    {
        explicit DestAccess(const VkAccessFlags& _flags)
            : flags(_flags)
        { }

        VkAccessFlags flags;
    };

    struct BarrierPoint
    {
        BarrierPoint(VkPipelineStageFlags _stage, VkAccessFlags _access)
            : stage(_stage)
            , access(_access)
        { }

        VkPipelineStageFlags stage;
        VkAccessFlags access;
    };

    struct ImageTransition
    {
        explicit ImageTransition(const VkImageLayout& _layout)
            : oldLayout(_layout)
            , newLayout(_layout)
        { }

        ImageTransition(const VkImageLayout& _oldLayout, const VkImageLayout& _newLayout)
            : oldLayout(_oldLayout)
            , newLayout(_newLayout)
        { }

        VkImageLayout oldLayout;
        VkImageLayout newLayout;
    };

    struct Layers
    {
        Layers(uint32_t _startLayer, uint32_t _numLayers)
            : startLayer(_startLayer)
            , numLayers(_numLayers)
        { }

        uint32_t startLayer;
        uint32_t numLayers;
    };

    struct Levels
    {
        Levels(uint32_t _baseLevel, uint32_t _levelCount)
            : baseLevel(_baseLevel)
            , levelCount(_levelCount)
        { }

        uint32_t baseLevel;
        uint32_t levelCount;
    };

    struct BufferMemoryBarrier
    {
        BufferMemoryBarrier(BufferPtr _buffer,
                            const std::size_t& _offset,
                            const std::size_t& _byteSize,
                            const SourceAccess& _sourceAccess,
                            const DestAccess& _destAccess)
            : buffer(std::move(_buffer))
            , offset(_offset)
            , byteSize(_byteSize)
            , sourceAccess(_sourceAccess)
            , destAccess(_destAccess)
        {}

        BufferPtr buffer;
        const std::size_t& offset;
        const std::size_t& byteSize;
        SourceAccess sourceAccess;
        DestAccess destAccess;
    };

    struct ImageAccess
    {
        ImageAccess(VkImageLayout _requiredInitialLayout,
                    VkImageLayout _finalLayout,
                    BarrierPoint _earliestUsage,
                    BarrierPoint _latestUsage,
                    Layers _layers,
                    Levels _levels,
                    VkImageAspectFlags _vkImageAspect);


        ImageAccess(BarrierPoint _earliestUsage,
                    BarrierPoint _latestUsage,
                    Layers _layers,
                    Levels _levels,
                    VkImageAspectFlags _vkImageAspect);

        VkImageLayout requiredInitialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
        VkImageLayout finalLayout{VK_IMAGE_LAYOUT_UNDEFINED};

        BarrierPoint earliestUsage;
        BarrierPoint latestUsage;

        Layers layers;
        Levels levels;
        VkImageAspectFlags vkImageAspect;
    };

    void InsertPipelineBarrier_Buffer(const IVulkanCallsPtr& vk,
                                      const VulkanCommandBufferPtr& commandBuffer,
                                      const SourceStage& sourceStage,
                                      const DestStage& destStage,
                                      const BufferMemoryBarrier& memoryBarrier);

    void InsertPipelineBarrier_Image(const IVulkanCallsPtr& vk,
                                     const IImagesPtr& images,
                                     const VulkanCommandBufferPtr& commandBuffer,
                                     const LoadedImage& loadedImage,
                                     const Layers& layers,
                                     const Levels& levels,
                                     const VkImageAspectFlags& vkImageAspectFlags,
                                     const BarrierPoint& source,
                                     const BarrierPoint& dest,
                                     const ImageTransition& imageTransition);
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_SYNCHRONIZATION_H
