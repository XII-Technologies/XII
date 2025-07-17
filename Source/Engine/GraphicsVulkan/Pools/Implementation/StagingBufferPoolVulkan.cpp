#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>

xiiGALStagingBufferPoolVulkan::xiiGALStagingBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags) :
  m_pDeviceVulkan(pDeviceVulkan), m_uiAlignment(uiAlignment), m_vkBufferUsageFlags(vkBufferUsageFlags)
{
}

xiiGALStagingBufferPoolVulkan::~xiiGALStagingBufferPoolVulkan()
{
  // We assume that these resources are not in use when this pool is destroyed.
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  for (auto& stagingBufferPages : m_StagingBufferPages)
  {
    pVulkanMemoryAllocator->DestroyBuffer(stagingBufferPages.m_vkBuffer, stagingBufferPages.m_VulkanAllocation);
  }
  m_StagingBufferPages.Clear();

  for (auto& largeAllocation : m_LargeAllocations)
  {
    pVulkanMemoryAllocator->DestroyBuffer(largeAllocation.m_vkBuffer, largeAllocation.m_VulkanAllocation);
  }
  m_LargeAllocations.Clear();
}

void xiiGALStagingBufferPoolVulkan::CreateStagingBufferPage()
{
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  StagingBufferPage stagingBufferPage;
  xiiUInt64         uiPageSize = s_uiStagingBufferDefaultPageSize;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiPageSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;
  allocationCreateInfo.m_Flags = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

  VK_ASSERT_DEV(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, stagingBufferPage.m_vkBuffer, stagingBufferPage.m_VulkanAllocation));

  stagingBufferPage.m_uiSize = uiPageSize;

  m_StagingBufferPages.PushBack(stagingBufferPage);
}

void xiiGALStagingBufferPoolVulkan::CreateLargeBuffer(xiiUInt64 uiSize)
{
  xiiVulkanMemoryAllocator* pVulkanMemoryAllocator = m_pDeviceVulkan->GetVulkanMemoryAllocator();

  StagingBufferPage stagingBufferPage;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  xiiVulkanAllocationCreateInfo allocationCreateInfo;
  allocationCreateInfo.m_Usage = xiiVulkanMemoryUsage::Auto;
  allocationCreateInfo.m_Flags = xiiVulkanAllocationCreateFlags::StrategyHostSequential;

  VK_ASSERT_DEV(pVulkanMemoryAllocator->CreateBuffer(vkBufferCreateInfo, allocationCreateInfo, stagingBufferPage.m_vkBuffer, stagingBufferPage.m_VulkanAllocation));

  stagingBufferPage.m_uiSize = uiSize;

  m_LargeAllocations.PushBack(stagingBufferPage);
}

xiiGALStagingBufferAllocationVulkan xiiGALStagingBufferPoolVulkan::Allocate(xiiUInt64 uiSize, bool bForceLargePage)
{
  if (bForceLargePage || uiSize >= (s_uiStagingBufferDefaultPageSize >> 2))
  {
    CreateLargeBuffer(uiSize);

    const auto& largeAllocation = m_LargeAllocations.PeekBack();

    xiiGALStagingBufferAllocationVulkan stagingBufferAllocation;
    stagingBufferAllocation.m_vkBuffer         = largeAllocation.m_vkBuffer;
    stagingBufferAllocation.m_VulkanAllocation = largeAllocation.m_VulkanAllocation;
    stagingBufferAllocation.m_uiOffset         = 0;

    return stagingBufferAllocation;
  }

  xiiUInt64 uiBufferAllocationOffset = xiiMemoryUtils::AlignSize(m_uiOffsetAllocationCounter, xiiUInt64{m_uiAlignment});
  xiiUInt32 uiBufferID               = xiiInvalidIndex;

  for (xiiUInt32 i = m_uiPageAllocationCounter; i < m_StagingBufferPages.GetCount(); ++i)
  {
    if ((uiBufferAllocationOffset + uiSize) <= m_StagingBufferPages[i].m_uiSize)
    {
      uiBufferID = i;
      break;
    }
    uiBufferAllocationOffset = 0U;
  }

  // If we cannot find an existing page with sufficient free space, create a new page.
  if (uiBufferID == xiiInvalidIndex)
  {
    CreateStagingBufferPage();

    uiBufferID = m_StagingBufferPages.GetCount() - 1;
  }

  // Sub-allocate from current page.
  xiiGALStagingBufferAllocationVulkan stagingBufferAllocation;
  stagingBufferAllocation.m_vkBuffer         = m_StagingBufferPages[uiBufferID].m_vkBuffer;
  stagingBufferAllocation.m_VulkanAllocation = m_StagingBufferPages[uiBufferID].m_VulkanAllocation;
  stagingBufferAllocation.m_uiOffset         = uiBufferAllocationOffset;

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return stagingBufferAllocation;
}

void xiiGALStagingBufferPoolVulkan::Reset()
{
  m_uiPageAllocationCounter   = 0;
  m_uiOffsetAllocationCounter = 0;

  for (auto& stagingBufferPages : m_StagingBufferPages)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(std::move(stagingBufferPages.m_vkBuffer), std::move(stagingBufferPages.m_VulkanAllocation));
  }
  m_StagingBufferPages.Clear();

  for (auto& largeAllocation : m_LargeAllocations)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(std::move(largeAllocation.m_vkBuffer), std::move(largeAllocation.m_VulkanAllocation));
  }
  m_LargeAllocations.Clear();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_StagingBufferPoolVulkan);
