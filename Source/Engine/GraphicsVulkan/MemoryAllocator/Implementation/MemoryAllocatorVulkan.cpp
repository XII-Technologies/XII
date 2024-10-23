#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4505) // Warning C4505 : 'swap' : unreferenced function with internal linkage has been removed.

#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

XII_WARNING_POP()

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_MemoryAllocator_Implementation_MemoryAllocator);
