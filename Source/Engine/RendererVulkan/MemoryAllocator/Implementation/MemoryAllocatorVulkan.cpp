#include <RendererVulkan/RendererVulkanPCH.h>


VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirements(
  VkDevice                                device,
  const VkDeviceBufferMemoryRequirements* pInfo,
  VkMemoryRequirements2*                  pMemoryRequirements)
{
  XII_REPORT_FAILURE("FIXME: Added to prevent the error: The procedure entry point vkGetDeviceBufferMemoryRequirements could not be located in the dynamic link library xiiRendererVulkan.dll.");
}

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#define VMA_VULKAN_VERSION           1001000
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED     1


//
//#define VMA_DEBUG_LOG(format, ...)   \
//  do                                 \
//  {                                  \
//    xiiStringBuilder tmp;             \
//    tmp.Printf(format, __VA_ARGS__); \
//    xiiLog::Error("{}", tmp);         \
//  } while (false)

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#define VMA_IMPLEMENTATION

#ifndef VA_IGNORE_THIS_FILE
#  define VA_INCLUDE_HIDDEN <vma/vk_mem_alloc.h>
#else
#  define VA_INCLUDE_HIDDEN ""
#endif

#include VA_INCLUDE_HIDDEN

XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == xiiVulkanAllocationCreateFlags::DedicatedMemory);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == xiiVulkanAllocationCreateFlags::NeverAllocate);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_MAPPED_BIT == xiiVulkanAllocationCreateFlags::Mapped);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT == xiiVulkanAllocationCreateFlags::CanAlias);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == xiiVulkanAllocationCreateFlags::HostAccessSequentialWrite);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == xiiVulkanAllocationCreateFlags::HostAccessRandom);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == xiiVulkanAllocationCreateFlags::StrategyMinMemory);
XII_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == xiiVulkanAllocationCreateFlags::StrategyMinTime);

XII_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_UNKNOWN == xiiVulkanMemoryUsage::Unknown);
XII_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == xiiVulkanMemoryUsage::GpuLazilyAllocated);
XII_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO == xiiVulkanMemoryUsage::Auto);
XII_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE == xiiVulkanMemoryUsage::AutoPreferDevice);
XII_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_HOST == xiiVulkanMemoryUsage::AutoPreferHost);

XII_CHECK_AT_COMPILETIME(sizeof(xiiVulkanAllocation) == sizeof(VmaAllocation));

XII_CHECK_AT_COMPILETIME(sizeof(xiiVulkanAllocationInfo) == sizeof(VmaAllocationInfo));


struct xiiMemoryAllocatorVulkan::Impl
{
  XII_DECLARE_POD_TYPE();
  VmaAllocator m_allocator;
};

xiiMemoryAllocatorVulkan::Impl* xiiMemoryAllocatorVulkan::m_pImpl = nullptr;

vk::Result xiiMemoryAllocatorVulkan::Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance)
{
  XII_ASSERT_DEV(m_pImpl == nullptr, "xiiMemoryAllocatorVulkan::Initialize was already called");
  m_pImpl = XII_DEFAULT_NEW(Impl);

  VmaVulkanFunctions vulkanFunctions    = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_1;
  allocatorCreateInfo.physicalDevice         = physicalDevice;
  allocatorCreateInfo.device                 = device;
  allocatorCreateInfo.instance               = instance;
  allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

  vk::Result res = (vk::Result)vmaCreateAllocator(&allocatorCreateInfo, &m_pImpl->m_allocator);
  if (res != vk::Result::eSuccess)
  {
    XII_DEFAULT_DELETE(m_pImpl);
  }
  return res;
}

void xiiMemoryAllocatorVulkan::DeInitialize()
{
  XII_ASSERT_DEV(m_pImpl != nullptr, "xiiMemoryAllocatorVulkan is not initialized.");

  vmaDestroyAllocator(m_pImpl->m_allocator);
  XII_DEFAULT_DELETE(m_pImpl);
}

vk::Result xiiMemoryAllocatorVulkan::CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, xiiVulkanAllocation& out_alloc, xiiVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage                   = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags                   = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateImage(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void xiiMemoryAllocatorVulkan::DestroyImage(vk::Image& image, xiiVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyImage(m_pImpl->m_allocator, reinterpret_cast<VkImage&>(image), reinterpret_cast<VmaAllocation&>(alloc));
  image = nullptr;
  alloc = nullptr;
}

vk::Result xiiMemoryAllocatorVulkan::CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, xiiVulkanAllocation& out_alloc, xiiVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage                   = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags                   = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateBuffer(m_pImpl->m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void xiiMemoryAllocatorVulkan::DestroyBuffer(vk::Buffer& buffer, xiiVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyBuffer(m_pImpl->m_allocator, reinterpret_cast<VkBuffer&>(buffer), reinterpret_cast<VmaAllocation&>(alloc));
  buffer = nullptr;
  alloc  = nullptr;
}

xiiVulkanAllocationInfo xiiMemoryAllocatorVulkan::GetAllocationInfo(xiiVulkanAllocation alloc)
{
  VmaAllocationInfo info;
  vmaGetAllocationInfo(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), &info);

  return reinterpret_cast<xiiVulkanAllocationInfo&>(info);
}

void xiiMemoryAllocatorVulkan::SetAllocationUserData(xiiVulkanAllocation alloc, const char* pUserData)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), (void*)pUserData);
}

vk::Result xiiMemoryAllocatorVulkan::MapMemory(xiiVulkanAllocation alloc, void** pData)
{
  return (vk::Result)vmaMapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), pData);
}

void xiiMemoryAllocatorVulkan::UnmapMemory(xiiVulkanAllocation alloc)
{
  vmaUnmapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc));
}

vk::Result xiiMemoryAllocatorVulkan::FlushAllocation(xiiVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaFlushAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}

vk::Result xiiMemoryAllocatorVulkan::InvalidateAllocation(xiiVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaInvalidateAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}
