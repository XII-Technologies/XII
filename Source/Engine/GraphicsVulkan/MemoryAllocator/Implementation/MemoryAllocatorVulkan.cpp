/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVulkanMemoryUsage, 1)
  XII_ENUM_CONSTANT(xiiVulkanMemoryUsage::Auto),
  XII_ENUM_CONSTANT(xiiVulkanMemoryUsage::AutoPreferDevice),
  XII_ENUM_CONSTANT(xiiVulkanMemoryUsage::AutoPreferHost),
  XII_ENUM_CONSTANT(xiiVulkanMemoryUsage::GpuLazilyAllocated),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiVulkanAllocationCreateFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::Dedicated),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::NeverAllocate),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::Mapped),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::UserDataCopy),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::UpperAddress),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyMinMemory),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyMinTime),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyFirstFit),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyCanAlias),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyWithinBudget),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyHostSequential),
  XII_BITFLAGS_CONSTANT(xiiVulkanAllocationCreateFlags::StrategyHostRandom),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiVulkanMemoryPropertyFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::DeviceLocal),
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::HostVisible),
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::HostCoherent),
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::HostCached),
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::LazilyAllocated),
  XII_BITFLAGS_CONSTANT(xiiVulkanMemoryPropertyFlags::Protected),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

//////////////////////////////////////////////////////////////////////////
// Helpers: map our flags/usage -> VMA

static_assert(sizeof(xiiVulkanAllocation) == sizeof(VmaAllocation));
static_assert(sizeof(xiiVulkanAllocationInfo) == sizeof(VmaAllocationInfo));

XII_DEFINE_AS_POD_TYPE(VmaBudget);

namespace
{
  struct ExportedSharedPool
  {
    VmaPool                                    m_Pool;                      ///< The VMA pool handle.
    xiiUniquePtr<vk::ExportMemoryAllocateInfo> m_pExportMemoryAllocateInfo; ///< The export memory allocate info structure. This must stay alive as long as the pool is used.
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    xiiUniquePtr<vk::ExportMemoryWin32HandleInfoKHR> m_pExportMemoryWin32HandleInfoKHR; ///< The export memory Win32 handle info structure.
#endif
  };

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

  XII_ALWAYS_INLINE static VmaAllocationCreateFlags ConvertFlags(xiiBitflags<xiiVulkanAllocationCreateFlags> flags)
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

  XII_ALWAYS_INLINE VkMemoryPropertyFlags ConvertMemoryPropertyFlags(xiiBitflags<xiiVulkanMemoryPropertyFlags> flags)
  {
    VkMemoryPropertyFlags vkMemoryPropertyFlags = 0U;

    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::DeviceLocal))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::HostVisible))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::HostCoherent))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::HostCached))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::LazilyAllocated))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    if (flags.IsSet(xiiVulkanMemoryPropertyFlags::Protected))
      vkMemoryPropertyFlags |= VK_MEMORY_PROPERTY_PROTECTED_BIT;

    return vkMemoryPropertyFlags;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////
// Implementation struct

struct xiiVulkanMemoryAllocator::Implementation
{
  VmaAllocator                       m_VmaAllocator = VK_NULL_HANDLE; ///< The VMA allocator handle.
  vk::Instance                       m_vkInstance;                    ///< The Vulkan instance.
  vk::PhysicalDevice                 m_vkPhysicalDevice;              ///< The Vulkan physical device.
  vk::Device                         m_vkLogicalDevice;               ///< The Vulkan logical device.
  vk::PhysicalDeviceMemoryProperties m_vkMemoryProperties;            ///< The physical device memory properties.

  xiiMutex                                    m_ExportedSharedPoolsMutex; ///< Mutex to protect access to the exported shared pools.
  xiiHashTable<xiiUInt32, ExportedSharedPool> m_ExportedSharedPools;      ///< Hashtable of exported shared pools, keyed by memory type index.
};

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor

xiiVulkanMemoryAllocator::xiiVulkanMemoryAllocator()
{
  m_pImplementation = XII_DEFAULT_NEW(xiiVulkanMemoryAllocator::Implementation);
}

xiiVulkanMemoryAllocator::~xiiVulkanMemoryAllocator()
{
  if (m_pImplementation->m_VmaAllocator != VK_NULL_HANDLE)
  {
    DeInitialize();
  }
}

//////////////////////////////////////////////////////////////////////////
// Initialize / DeInitialize

vk::Result xiiVulkanMemoryAllocator::Initialize(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiPreferredBlockSize /*= 0U*/)
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

  VmaAllocatorCreateInfo vmaAllocatorCreateInfo      = {};
  vmaAllocatorCreateInfo.vulkanApiVersion            = physicalDeviceProperties.apiVersion;
  vmaAllocatorCreateInfo.instance                    = m_pImplementation->m_vkInstance;
  vmaAllocatorCreateInfo.physicalDevice              = m_pImplementation->m_vkPhysicalDevice;
  vmaAllocatorCreateInfo.device                      = m_pImplementation->m_vkLogicalDevice;
  vmaAllocatorCreateInfo.preferredLargeHeapBlockSize = uiPreferredBlockSize;
  vmaAllocatorCreateInfo.pVulkanFunctions            = &vmaVulkanFunctions;
  vmaAllocatorCreateInfo.flags                       = {};

  const xiiGALDeviceVulkan::ExtensionFeatures& logicalDeviceExtensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

  if (logicalDeviceExtensionFeatures.m_BufferDeviceAddress.bufferDeviceAddress == vk::True)
  {
    // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION is required by our implementation for ray tracing.
    vmaAllocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  }

  return static_cast<vk::Result>(vmaCreateAllocator(&vmaAllocatorCreateInfo, &m_pImplementation->m_VmaAllocator));
}

void xiiVulkanMemoryAllocator::DeInitialize()
{
  XII_ASSERT_DEV(m_pImplementation != nullptr && m_pImplementation->m_VmaAllocator != VK_NULL_HANDLE, "xiiVulkanMemoryAllocator not initialized or already de-initialized.");

  for (auto& it : m_pImplementation->m_ExportedSharedPools)
  {
    vmaDestroyPool(m_pImplementation->m_VmaAllocator, it.Value().m_Pool);
  }
  m_pImplementation->m_ExportedSharedPools.Clear();

  vmaDestroyAllocator(m_pImplementation->m_VmaAllocator);
  m_pImplementation->m_VmaAllocator = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// Create / Destroy Buffer

vk::Result xiiVulkanMemoryAllocator::CreateBuffer(const vk::BufferCreateInfo& vkBufferCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo) const
{
  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = ConvertUsage(allocationCreateInfo.m_Usage);
  vmaAllocationCreateInfo.flags                   = ConvertFlags(allocationCreateInfo.m_Flags);
  vmaAllocationCreateInfo.requiredFlags           = ConvertMemoryPropertyFlags(allocationCreateInfo.m_RequiredFlags);
  vmaAllocationCreateInfo.preferredFlags          = ConvertMemoryPropertyFlags(allocationCreateInfo.m_PreferredFlags);
  vmaAllocationCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    vmaAllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    XII_LOCK(m_pImplementation->m_ExportedSharedPoolsMutex);

    xiiUInt32 uiMemoryTypeIndex = 0U;
    if (VkResult vkResult = vmaFindMemoryTypeIndexForBufferInfo(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, &uiMemoryTypeIndex); vkResult != VK_SUCCESS)
    {
      return vk::Result{vkResult};
    }

    ExportedSharedPool* pExportedSharedPool = m_pImplementation->m_ExportedSharedPools.GetValue(uiMemoryTypeIndex);
    if (pExportedSharedPool == nullptr)
    {
      ExportedSharedPool newExportedSharedPool;
      {
        newExportedSharedPool.m_pExportMemoryAllocateInfo = XII_DEFAULT_NEW(vk::ExportMemoryAllocateInfo);

#if XII_ENABLED(XII_PLATFORM_LINUX)
        newExportedSharedPool.m_pExportMemoryAllocateInfo->handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)

        newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR           = XII_DEFAULT_NEW(vk::ExportMemoryWin32HandleInfoKHR);
        newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR->dwAccess = GENERIC_READ | GENERIC_WRITE;

        newExportedSharedPool.m_pExportMemoryAllocateInfo->handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        newExportedSharedPool.m_pExportMemoryAllocateInfo->pNext       = newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR.Borrow();
#else
        xiiLog::Error("Exportable shared Vulkan buffer allocations are unsupported on this platform.");
        return vk::Result::eErrorFeatureNotPresent;
#endif
      }

      VmaPoolCreateInfo vmaPoolCreateInfo   = {};
      vmaPoolCreateInfo.memoryTypeIndex     = uiMemoryTypeIndex;
      vmaPoolCreateInfo.pMemoryAllocateNext = newExportedSharedPool.m_pExportMemoryAllocateInfo.Borrow();

      VmaPool vmaPool;
      if (VkResult vkResult = vmaCreatePool(m_pImplementation->m_VmaAllocator, &vmaPoolCreateInfo, &vmaPool); vkResult != VK_SUCCESS)
      {
        return vk::Result{vkResult};
      }

      newExportedSharedPool.m_Pool = vmaPool;

      XII_VERIFY(m_pImplementation->m_ExportedSharedPools.Insert(uiMemoryTypeIndex, std::move(newExportedSharedPool)), "Failed to insert exported shared pool.");

      pExportedSharedPool = m_pImplementation->m_ExportedSharedPools.GetValue(uiMemoryTypeIndex);
    }

    vmaAllocationCreateInfo.pool = pExportedSharedPool->m_Pool;
  }

  return static_cast<vk::Result>(vmaCreateBuffer(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_allocation), reinterpret_cast<VmaAllocationInfo*>(pAllocationInfo)));
}

void xiiVulkanMemoryAllocator::DestroyBuffer(vk::Buffer& vkBuffer, xiiVulkanAllocation& allocation) const
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), nullptr);

  vmaDestroyBuffer(m_pImplementation->m_VmaAllocator, reinterpret_cast<VkBuffer&>(vkBuffer), reinterpret_cast<VmaAllocation&>(allocation));

  vkBuffer   = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// Create / Destroy Image

vk::Result xiiVulkanMemoryAllocator::CreateImage(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo) const
{
  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = ConvertUsage(allocationCreateInfo.m_Usage);
  vmaAllocationCreateInfo.flags                   = ConvertFlags(allocationCreateInfo.m_Flags);
  vmaAllocationCreateInfo.requiredFlags           = ConvertMemoryPropertyFlags(allocationCreateInfo.m_RequiredFlags);
  vmaAllocationCreateInfo.preferredFlags          = ConvertMemoryPropertyFlags(allocationCreateInfo.m_PreferredFlags);
  vmaAllocationCreateInfo.pUserData               = (void*)allocationCreateInfo.m_pUserData;

  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    vmaAllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    XII_LOCK(m_pImplementation->m_ExportedSharedPoolsMutex);

    xiiUInt32 uiMemoryTypeIndex = 0U;
    if (VkResult vkResult = vmaFindMemoryTypeIndexForImageInfo(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, &uiMemoryTypeIndex); vkResult != VK_SUCCESS)
    {
      return vk::Result{vkResult};
    }

    ExportedSharedPool* pExportedSharedPool = m_pImplementation->m_ExportedSharedPools.GetValue(uiMemoryTypeIndex);
    if (pExportedSharedPool == nullptr)
    {
      ExportedSharedPool newExportedSharedPool;
      {
        newExportedSharedPool.m_pExportMemoryAllocateInfo = XII_DEFAULT_NEW(vk::ExportMemoryAllocateInfo);

#if XII_ENABLED(XII_PLATFORM_LINUX)
        newExportedSharedPool.m_pExportMemoryAllocateInfo->handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)

        newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR           = XII_DEFAULT_NEW(vk::ExportMemoryWin32HandleInfoKHR);
        newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR->dwAccess = GENERIC_READ | GENERIC_WRITE;

        newExportedSharedPool.m_pExportMemoryAllocateInfo->handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        newExportedSharedPool.m_pExportMemoryAllocateInfo->pNext       = newExportedSharedPool.m_pExportMemoryWin32HandleInfoKHR.Borrow();
#else
        xiiLog::Error("Exportable shared Vulkan image allocations are unsupported on this platform.");
        return vk::Result::eErrorFeatureNotPresent;
#endif
      }

      VmaPoolCreateInfo vmaPoolCreateInfo   = {};
      vmaPoolCreateInfo.memoryTypeIndex     = uiMemoryTypeIndex;
      vmaPoolCreateInfo.pMemoryAllocateNext = newExportedSharedPool.m_pExportMemoryAllocateInfo.Borrow();

      VmaPool vmaPool;
      if (VkResult vkResult = vmaCreatePool(m_pImplementation->m_VmaAllocator, &vmaPoolCreateInfo, &vmaPool); vkResult != VK_SUCCESS)
      {
        return vk::Result{vkResult};
      }

      newExportedSharedPool.m_Pool = vmaPool;

      XII_VERIFY(m_pImplementation->m_ExportedSharedPools.Insert(uiMemoryTypeIndex, std::move(newExportedSharedPool)), "Failed to insert exported shared pool.");

      pExportedSharedPool = m_pImplementation->m_ExportedSharedPools.GetValue(uiMemoryTypeIndex);
    }

    vmaAllocationCreateInfo.pool = pExportedSharedPool->m_Pool;
  }

  return static_cast<vk::Result>(vmaCreateImage(m_pImplementation->m_VmaAllocator, reinterpret_cast<const VkImageCreateInfo*>(&vkImageCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_allocation), reinterpret_cast<VmaAllocationInfo*>(pAllocationInfo)));
}

void xiiVulkanMemoryAllocator::DestroyImage(vk::Image& vkImage, xiiVulkanAllocation& allocation) const
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), nullptr);

  vmaDestroyImage(m_pImplementation->m_VmaAllocator, reinterpret_cast<VkImage&>(vkImage), reinterpret_cast<VmaAllocation&>(allocation));

  vkImage    = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////
// Info / UserData

xiiVulkanAllocationInfo xiiVulkanMemoryAllocator::GetAllocationInfo(xiiVulkanAllocation allocation) const
{
  VmaAllocationInfo vmaAllocationInfo;

  vmaGetAllocationInfo(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), &vmaAllocationInfo);

  return reinterpret_cast<xiiVulkanAllocationInfo&>(vmaAllocationInfo);
}

vk::MemoryPropertyFlags xiiVulkanMemoryAllocator::GetMemoryPropertyFlags(xiiVulkanAllocation allocation) const
{
  VkMemoryPropertyFlags vkMemoryPropertyFlags;

  vmaGetAllocationMemoryProperties(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), &vkMemoryPropertyFlags);

  return reinterpret_cast<vk::MemoryPropertyFlags&>(vkMemoryPropertyFlags);
}

void xiiVulkanMemoryAllocator::SetAllocationUserData(xiiVulkanAllocation allocation, const char* pUserData) const
{
  vmaSetAllocationUserData(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), (void*)pUserData);
}

//////////////////////////////////////////////////////////////////////////
// Map / Unmap / Flush / Invalidate

vk::Result xiiVulkanMemoryAllocator::MapMemory(xiiVulkanAllocation allocation, void** pData) const
{
  return static_cast<vk::Result>(vmaMapMemory(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), pData));
}

void xiiVulkanMemoryAllocator::UnmapMemory(xiiVulkanAllocation allocation) const
{
  vmaUnmapMemory(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation));
}

vk::Result xiiVulkanMemoryAllocator::FlushAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size) const
{
  return static_cast<vk::Result>(vmaFlushAllocation(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), offset, size));
}

vk::Result xiiVulkanMemoryAllocator::InvalidateAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size) const
{
  return static_cast<vk::Result>(vmaInvalidateAllocation(m_pImplementation->m_VmaAllocator, reinterpret_cast<VmaAllocation&>(allocation), offset, size));
}

xiiVulkanMemoryStatistics xiiVulkanMemoryAllocator::GetStatistics() const
{
  xiiVulkanMemoryStatistics vkMemoryStatistics;

  xiiTemporaryHybridArray<VmaBudget, 4U> vmaBudgets;
  vmaBudgets.SetCount(m_pImplementation->m_vkMemoryProperties.memoryHeapCount);

  vmaGetHeapBudgets(m_pImplementation->m_VmaAllocator, vmaBudgets.GetData());

  for (xiiUInt32 i = 0; i < m_pImplementation->m_vkMemoryProperties.memoryHeapCount; ++i)
  {
    const VmaBudget& budget = vmaBudgets[i];

    vkMemoryStatistics.m_uiBlockCount += budget.statistics.blockCount;
    vkMemoryStatistics.m_uiBlockBytes += static_cast<xiiUInt64>(budget.statistics.blockBytes);
    vkMemoryStatistics.m_uiAllocationCount += budget.statistics.allocationCount;
    vkMemoryStatistics.m_uiAllocationBytes += static_cast<xiiUInt64>(budget.statistics.allocationBytes);
  }

  return vkMemoryStatistics;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_MemoryAllocator_Implementation_MemoryAllocatorVulkan);
