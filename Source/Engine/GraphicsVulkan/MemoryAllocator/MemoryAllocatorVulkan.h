#include <GraphicsVulkan/GraphicsVulkanDLL.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_CLANG("-Wnullability-completeness")
XII_WARNING_DISABLE_CLANG("-Wunused-variable")
XII_WARNING_DISABLE_CLANG("-Wunused-private-field")

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

XII_WARNING_POP()
