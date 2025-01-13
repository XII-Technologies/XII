#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPool.h>

xiiGALStagingBufferPoolVulkan::xiiGALStagingBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags) :
  m_pDeviceVulkan(pDeviceVulkan), m_uiAlignment(uiAlignment), m_vkBufferUsageFlags(vkBufferUsageFlags)
{
}

xiiGALStagingBufferPoolVulkan::~xiiGALStagingBufferPoolVulkan()
{
  for (const auto& stagingBufferPages : m_StagingBufferPages)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(stagingBufferPages.m_vkBuffer);
  }
  m_StagingBufferPages.Clear();

  for (const auto& largeAllocation : m_LargeAllocations)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(largeAllocation.m_vkBuffer);
  }
  m_LargeAllocations.Clear();
}

void xiiGALStagingBufferPoolVulkan::CreateStagingBufferPage()
{
  StagingBufferPage stagingBufferPage;
  xiiUInt64         uiPageSize = s_uiStagingBufferDefaultPageSize;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiPageSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;

  VK_ASSERT_DEV(vmaCreateBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&stagingBufferPage.m_vkBuffer), &stagingBufferPage.m_VmaAllocation, nullptr));

  stagingBufferPage.m_uiSize = uiPageSize;

  m_StagingBufferPages.PushBack(stagingBufferPage);
}

void xiiGALStagingBufferPoolVulkan::CreateLargeBuffer(xiiUInt64 uiSize)
{
  StagingBufferPage stagingBufferPage;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;

  VK_ASSERT_DEV(vmaCreateBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&stagingBufferPage.m_vkBuffer), &stagingBufferPage.m_VmaAllocation, nullptr));

  stagingBufferPage.m_uiSize = uiSize;

  m_LargeAllocations.PushBack(stagingBufferPage);
}

xiiGALStagingBufferPoolVulkan::Allocation xiiGALStagingBufferPoolVulkan::Allocate(xiiUInt64 uiSize, bool bForceLargePage)
{
  if (bForceLargePage || uiSize >= (s_uiStagingBufferDefaultPageSize >> 2))
  {
    CreateLargeBuffer(uiSize);

    xiiGALStagingBufferPoolVulkan::Allocation stagingBufferAllocation;
    stagingBufferAllocation.m_vkBuffer = m_LargeAllocations.PeekBack().m_vkBuffer;
    stagingBufferAllocation.m_uiOffset = 0;

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
    uiBufferAllocationOffset = 0;
  }

  // If we cannot find an existing page with sufficient free space, create a new page.
  if (uiBufferID == xiiInvalidIndex)
  {
    CreateStagingBufferPage();

    uiBufferID = m_StagingBufferPages.GetCount() - 1;
  }

  // Sub allocate from current page.
  xiiGALStagingBufferPoolVulkan::Allocation stagingBufferAllocation;
  stagingBufferAllocation.m_vkBuffer = m_StagingBufferPages[uiBufferID].m_vkBuffer;
  stagingBufferAllocation.m_uiOffset = uiBufferAllocationOffset;

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return stagingBufferAllocation;
}

void xiiGALStagingBufferPoolVulkan::Reset()
{
  m_uiPageAllocationCounter   = 0;
  m_uiOffsetAllocationCounter = 0;

  for (const auto& largeAllocation : m_LargeAllocations)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(largeAllocation.m_vkBuffer);
  }
  m_LargeAllocations.Clear();
}
