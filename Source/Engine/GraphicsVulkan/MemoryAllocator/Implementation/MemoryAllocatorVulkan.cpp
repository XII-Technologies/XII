#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4100) // Warning C4100 : unreferenced formal parameter.
XII_WARNING_DISABLE_MSVC(4189) // Warning C4189 : local variable is initialized but not referenced.
XII_WARNING_DISABLE_MSVC(4505) // Warning C4505 : unreferenced function with internal linkage has been removed.
XII_WARNING_DISABLE_CLANG("-Wnullability-completeness")
XII_WARNING_DISABLE_CLANG("-Wunused-variable")
XII_WARNING_DISABLE_CLANG("-Wunused-private-field")

#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

XII_WARNING_POP()

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_MemoryAllocator_Implementation_MemoryAllocator);
