#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/DynamicBufferPoolVulkan.h>

xiiGALDynamicBufferPoolVulkan::xiiGALDynamicBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags) :
  m_pDeviceVulkan(pDeviceVulkan), m_uiAlignment(uiAlignment), m_vkBufferUsageFlags(vkBufferUsageFlags)
{
}

xiiGALDynamicBufferPoolVulkan::~xiiGALDynamicBufferPoolVulkan()
{
  // We assume that these resources are not in use when this pool is destroyed.
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  for (auto& dynamicBufferPages : m_DynamicBufferPages)
  {
    pVulkanMemoryAllocator->DestroyBuffer(dynamicBufferPages.m_vkBuffer, dynamicBufferPages.m_VulkanAllocation);
  }
  m_DynamicBufferPages.Clear();

  for (auto& largeAllocation : m_LargeAllocations)
  {
    pVulkanMemoryAllocator->DestroyBuffer(largeAllocation.m_vkBuffer, largeAllocation.m_VulkanAllocation);
  }
  m_LargeAllocations.Clear();
}

void xiiGALDynamicBufferPoolVulkan::CreateDynamicBufferPage()
{
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  DynamicBufferPage dynamicBufferPage;
  xiiUInt64         uiPageSize = s_uiDynamicBufferDefaultPageSize;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiPageSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;
  allocationCreateInfo.m_Flags = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

  VK_ASSERT_DEV(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, dynamicBufferPage.m_vkBuffer, dynamicBufferPage.m_VulkanAllocation));

  dynamicBufferPage.m_uiSize = uiPageSize;

  m_DynamicBufferPages.PushBack(dynamicBufferPage);
}

void xiiGALDynamicBufferPoolVulkan::CreateLargeBuffer(xiiUInt64 uiSize)
{
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  DynamicBufferPage dynamicBufferPage;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;
  allocationCreateInfo.m_Flags = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

  VK_ASSERT_DEV(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, dynamicBufferPage.m_vkBuffer, dynamicBufferPage.m_VulkanAllocation));

  dynamicBufferPage.m_uiSize = uiSize;

  m_LargeAllocations.PushBack(dynamicBufferPage);
}

xiiGALDynamicBufferAllocationVulkan xiiGALDynamicBufferPoolVulkan::Allocate(xiiUInt64 uiSize, bool bForceLargePage)
{
  if (bForceLargePage || uiSize >= (s_uiDynamicBufferDefaultPageSize >> 2))
  {
    CreateLargeBuffer(uiSize);

    const auto& largeAllocation = m_LargeAllocations.PeekBack();

    xiiGALDynamicBufferAllocationVulkan dynamicBufferAllocation;
    dynamicBufferAllocation.m_vkBuffer         = largeAllocation.m_vkBuffer;
    dynamicBufferAllocation.m_VulkanAllocation = largeAllocation.m_VulkanAllocation;
    dynamicBufferAllocation.m_uiOffset         = 0U;

    return dynamicBufferAllocation;
  }

  xiiUInt64 uiBufferAllocationOffset = xiiMemoryUtils::AlignSize(m_uiOffsetAllocationCounter, xiiUInt64{m_uiAlignment});
  xiiUInt32 uiBufferID               = xiiInvalidIndex;

  for (xiiUInt32 i = m_uiPageAllocationCounter; i < m_DynamicBufferPages.GetCount(); ++i)
  {
    if ((uiBufferAllocationOffset + uiSize) <= m_DynamicBufferPages[i].m_uiSize)
    {
      uiBufferID = i;
      break;
    }
    uiBufferAllocationOffset = 0;
  }

  // If we cannot find an existing page with sufficient free space, create a new page.
  if (uiBufferID == xiiInvalidIndex)
  {
    CreateDynamicBufferPage();

    uiBufferID = m_DynamicBufferPages.GetCount() - 1;
  }

  // Sub-allocate from current page.
  xiiGALDynamicBufferAllocationVulkan dynamicBufferAllocation;
  dynamicBufferAllocation.m_vkBuffer         = m_DynamicBufferPages[uiBufferID].m_vkBuffer;
  dynamicBufferAllocation.m_VulkanAllocation = m_DynamicBufferPages[uiBufferID].m_VulkanAllocation;
  dynamicBufferAllocation.m_uiOffset         = uiBufferAllocationOffset;

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return dynamicBufferAllocation;
}

void xiiGALDynamicBufferPoolVulkan::Reset()
{
  m_uiPageAllocationCounter   = 0;
  m_uiOffsetAllocationCounter = 0;

  for (auto& dynamicBufferPages : m_DynamicBufferPages)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(std::move(dynamicBufferPages.m_vkBuffer), std::move(dynamicBufferPages.m_VulkanAllocation));
  }
  m_DynamicBufferPages.Clear();

  for (auto& largeAllocation : m_LargeAllocations)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(std::move(largeAllocation.m_vkBuffer), std::move(largeAllocation.m_VulkanAllocation));
  }
  m_LargeAllocations.Clear();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_DynamicBufferPoolVulkan);
