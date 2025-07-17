#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4100) // Warning C4100 : unreferenced formal parameter.
XII_WARNING_DISABLE_MSVC(4189) // Warning C4189 : local variable is initialized but not referenced.
XII_WARNING_DISABLE_MSVC(4505) // Warning C4505 : unreferenced function with internal linkage has been removed.
XII_WARNING_DISABLE_CLANG("-Wnullability-completeness")
XII_WARNING_DISABLE_CLANG("-Wunused-variable")
XII_WARNING_DISABLE_CLANG("-Wunused-private-field")

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION           VK_API_VERSION_1_0 // Equivalent to 1000000.
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED     1

#include <VulkanMemoryAllocator/vk_mem_alloc.h>

XII_WARNING_POP()

//////////////////////////////////////////////////////////////////////////
// Helpers: map our flags/usage -> VMA

static_assert(sizeof(xiiVulkanAllocation) == sizeof(VmaAllocation));
static_assert(sizeof(xiiVulkanAllocationInfo) == sizeof(VmaAllocationInfo));

XII_DEFINE_AS_POD_TYPE(VmaBudget);

namespace
{
  XII_ALWAYS_INLINE static VmaMemoryUsage ConvertUsage(xiiVulkanMemoryUsage::Enum usage)
  {
    switch (usage)
    {
      case xiiVulkanMemoryUsage::Auto:
        return VMA_MEMORY_USAGE_AUTO;
      case xiiVulkanMemoryUsage::AutoPreferDevice:
        return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
      case xiiVulkanMemoryUsage::AutoPreferHost:
        return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
      case xiiVulkanMemoryUsage::GpuLazilyAllocated:
        return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
    return VMA_MEMORY_USAGE_UNKNOWN;
  }

  XII_ALWAYS_INLINE static VmaAllocationCreateFlags ConvertFlags(const xiiBitflags<xiiVulkanAllocationCreateFlags>& flags)
  {
    VmaAllocationCreateFlags allocationCreateFlags = 0U;

    if (flags.IsSet(xiiVulkanAllocationCreateFlags::Dedicated))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::NeverAllocate))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::Mapped))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::UserDataCopy))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::UpperAddress))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyMinMemory))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyMinTime))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyFirstFit))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyCanAlias))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyWithinBudget))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyHostSequential))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if (flags.IsSet(xiiVulkanAllocationCreateFlags::StrategyHostRandom))
      allocationCreateFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    return allocationCreateFlags;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////
// Implementation struct

struct xiiVulkanMemoryAllocator::Implementation
{
  VmaAllocator                       m_VmaAllocator = VK_NULL_HANDLE;
  vk::Instance                       m_vkInstance;
  vk::PhysicalDevice                 m_vkPhysicalDevice;
  vk::Device                         m_vkLogicalDevice;
  vk::PhysicalDeviceMemoryProperties m_vkMemoryProperties;
};

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor

xiiVulkanMemoryAllocator::xiiVulkanMemoryAllocator()
{
  m_pImplementation = XII_DEFAULT_NEW(xiiVulkanMemoryAllocator::Implementation);
}

xiiVulkanMemoryAllocator::~xiiVulkanMemoryAllocator() = default;

//////////////////////////////////////////////////////////////////////////
// Initialize / DeInitialize

vk::Result xiiVulkanMemoryAllocator::Initialize(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiPreferredBlockSize)
{
  // We prefer dynamically finding the function pointers.

  m_pImplementation->m_vkInstance         = pDeviceVulkan->GetVulkanInstance();
  m_pImplementation->m_vkPhysicalDevice   = pDeviceVulkan->GetVulkanPhysicalDevice();
  m_pImplementation->m_vkLogicalDevice    = pDeviceVulkan->GetVulkanLogicalDevice();
  m_pImplementation->m_vkMemoryProperties = pDeviceVulkan->GetVulkanPhysicalDeviceMemoryProperties();

  const vk::detail::DispatchLoaderDynamic& dynamicDispatchLoader    = pDeviceVulkan->GetVulkanDynamicDispatchLoader();
  const vk::PhysicalDeviceProperties&      physicalDeviceProperties = pDeviceVulkan->GetVulkanPhysicalDeviceProperties();

  VmaVulkanFunctions vmaVulkanFunctions    = {};
  vmaVulkanFunctions.vkGetInstanceProcAddr = dynamicDispatchLoader.vkGetInstanceProcAddr;
  vmaVulkanFunctions.vkGetDeviceProcAddr   = dynamicDispatchLoader.vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo vmaAllocatorCreateInfo = {};
  vmaAllocatorCreateInfo.vulkanApiVersion       = physicalDeviceProperties.apiVersion;
  vmaAllocatorCreateInfo.instance               = m_pImplementation->m_vkInstance;
  vmaAllocatorCreateInfo.physicalDevice         = m_pImplementation->m_vkPhysicalDevice;
  vmaAllocatorCreateInfo.device                 = m_pImplementation->m_vkLogicalDevice;
  vmaAllocatorCreateInfo.pVulkanFunctions       = &vmaVulkanFunctions;
  vmaAllocatorCreateInfo.flags                  = {};

  const xiiGALDeviceVulkan::ExtensionFeatures& physicalDeviceExtensionFeatures = pDeviceVulkan->GetPhysicalDeviceExtensionFeatures();

  if (physicalDeviceExtensionFeatures.m_BufferDeviceAddress.bufferDeviceAddress == vk::True)
  {
    // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION is required by our implementation for ray tracing.
    vmaAllocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  }

  VK_SUCCEED_OR_RETURN(vmaCreateAllocator(&vmaAllocatorCreateInfo, &m_pImplementation->m_VmaAllocator));

  return vk::Result::eSuccess;
}

void xiiVulkanMemoryAllocator::DeInitialize()
{
}

//////////////////////////////////////////////////////////////////////////
// Create / Destroy Buffer

vk::Result xiiVulkanMemoryAllocator::CreateBuffer(const vk::BufferCreateInfo& vkBufferCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo)
{
  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = ConvertUsage(allocationCreateInfo.m_Usage);
  vmaAllocationCreateInfo.flags                   = ConvertFlags(allocationCreateInfo.m_Flags);
  vmaAllocationCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  return static_cast<vk::Result>(vmaCreateBuffer(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_allocation), reinterpret_cast<VmaAllocationInfo*>(pAllocationInfo)));
}

void xiiVulkanMemoryAllocator::DestroyBuffer(vk::Buffer& vkBuffer, xiiVulkanAllocation& allocation)
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), nullptr);

  vmaDestroyBuffer(m_pImplementation->m_VmaAllocator, reinterpret_cast<VkBuffer&>(vkBuffer), reinterpret_cast<VmaAllocation&>(allocation));

  vkBuffer   = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// Create / Destroy Image

vk::Result xiiVulkanMemoryAllocator::CreateImage(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo)
{
  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = ConvertUsage(allocationCreateInfo.m_Usage);
  vmaAllocationCreateInfo.flags                   = ConvertFlags(allocationCreateInfo.m_Flags);
  vmaAllocationCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  return static_cast<vk::Result>(vmaCreateImage(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_allocation), reinterpret_cast<VmaAllocationInfo*>(pAllocationInfo)));
}

void xiiVulkanMemoryAllocator::DestroyImage(vk::Image& vkImage, xiiVulkanAllocation& allocation)
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), nullptr);

  vmaDestroyImage(m_pImplementation->m_VmaAllocator, reinterpret_cast<VkImage&>(vkImage), reinterpret_cast<VmaAllocation&>(allocation));

  vkImage    = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// Info / UserData

xiiVulkanAllocationInfo xiiVulkanMemoryAllocator::GetAllocationInfo(xiiVulkanAllocation allocation)
{
  VmaAllocationInfo vmaAllocationInfo;

  vmaGetAllocationInfo(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), &vmaAllocationInfo);

  return reinterpret_cast<xiiVulkanAllocationInfo&>(vmaAllocationInfo);
}

vk::MemoryPropertyFlags xiiVulkanMemoryAllocator::GetMemoryPropertyFlags(xiiVulkanAllocation allocation)
{
  VkMemoryPropertyFlags vkMemoryPropertyFlags;

  vmaGetAllocationMemoryProperties(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), &vkMemoryPropertyFlags);

  return reinterpret_cast<vk::MemoryPropertyFlags&>(vkMemoryPropertyFlags);
}

void xiiVulkanMemoryAllocator::SetAllocationUserData(xiiVulkanAllocation allocation, const char* pUserData)
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), (void*)pUserData);
}

//////////////////////////////////////////////////////////////////////////
// Map / Unmap / Flush / Invalidate

vk::Result xiiVulkanMemoryAllocator::MapMemory(xiiVulkanAllocation allocation, void** pData)
{
  return static_cast<vk::Result>(vmaMapMemory(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), pData));
}

void xiiVulkanMemoryAllocator::UnmapMemory(xiiVulkanAllocation allocation)
{
  vmaUnmapMemory(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation));
}

vk::Result xiiVulkanMemoryAllocator::FlushAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size)
{
  return static_cast<vk::Result>(vmaFlushAllocation(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), offset, size));
}

vk::Result xiiVulkanMemoryAllocator::InvalidateAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size)
{
  return static_cast<vk::Result>(vmaInvalidateAllocation(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), offset, size));
}

xiiVulkanMemoryStatistics xiiVulkanMemoryAllocator::GetStatistics()
{
  xiiVulkanMemoryStatistics vkMemoryStatistics;

  const xiiUInt32 uiHeapCount = m_pImplementation->m_VmaAllocator->GetMemoryHeapCount();

  xiiHybridArray<VmaBudget, 4U> vmaBudgets;
  vmaBudgets.SetCount(uiHeapCount);

  vmaGetHeapBudgets(m_pImplementation->m_VmaAllocator, vmaBudgets.GetData());

  for (xiiUInt32 i = 0; i < uiHeapCount; ++i)
  {
    const VmaBudget& budget = vmaBudgets[i];

    vkMemoryStatistics.m_uiBlockCount += budget.statistics.blockCount;
    vkMemoryStatistics.m_uiBlockBytes += static_cast<xiiUInt64>(budget.statistics.blockBytes);
    vkMemoryStatistics.m_uiAllocationCount += budget.statistics.allocationCount;
    vkMemoryStatistics.m_uiAllocationBytes += static_cast<xiiUInt64>(budget.statistics.allocationBytes);
  }

  return vkMemoryStatistics;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_MemoryAllocator_Implementation_MemoryAllocator);
