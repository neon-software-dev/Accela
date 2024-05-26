/**
 * Wrapper include for the Vulkan Memory Allocator header file. All usages of the VMA must include
 * this file, rather than directly including vm_mem_alloc.h
 */

//
// Set global VMA/Vulkan defines to configure the library
//
#define VMA_STATIC_VULKAN_FUNCTIONS 0   // We don't statically link to Vulkan
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0  // We don't provide global vkGetInstanceProcAddr/vkGetDeviceProcAddr funcs
#define VK_NO_PROTOTYPES                // We dynamically link to vulkan and don't need function prototypes exposed

#include "vk_mem_alloc.h"
