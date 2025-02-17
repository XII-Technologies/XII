#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/DynamicBufferPoolVulkan.h>

xiiGALDynamicBufferPoolVulkan::xiiGALDynamicBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags) :
  m_pDeviceVulkan(pDeviceVulkan), m_uiAlignment(uiAlignment), m_vkBufferUsageFlags(vkBufferUsageFlags)
{
}

xiiGALDynamicBufferPoolVulkan::~xiiGALDynamicBufferPoolVulkan()
{
  // We assume that these resources are not in use when this pool is destroyed.

  for (const auto& stagingBufferPages : m_DynamicBufferPages)
  {
    vmaDestroyBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), stagingBufferPages.m_vkBuffer, stagingBufferPages.m_VmaAllocation);
  }
  m_DynamicBufferPages.Clear();

  for (const auto& largeAllocation : m_LargeAllocations)
  {
    vmaDestroyBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), largeAllocation.m_vkBuffer, largeAllocation.m_VmaAllocation);
  }
  m_LargeAllocations.Clear();
}

void xiiGALDynamicBufferPoolVulkan::CreateDynamicBufferPage()
{
  DynamicBufferPage stagingBufferPage;
  xiiUInt64         uiPageSize = s_uiDynamicBufferDefaultPageSize;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiPageSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
  vmaAllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VK_ASSERT_DEV(vmaCreateBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&stagingBufferPage.m_vkBuffer), &stagingBufferPage.m_VmaAllocation, nullptr));

  stagingBufferPage.m_uiSize = uiPageSize;

  m_DynamicBufferPages.PushBack(stagingBufferPage);
}

void xiiGALDynamicBufferPoolVulkan::CreateLargeBuffer(xiiUInt64 uiSize)
{
  DynamicBufferPage stagingBufferPage;

  vk::BufferCreateInfo vkBufferCreateInfo = {};
  vkBufferCreateInfo.pNext                = nullptr;
  vkBufferCreateInfo.flags                = {};
  vkBufferCreateInfo.size                 = uiSize;
  vkBufferCreateInfo.usage                = m_vkBufferUsageFlags | vk::BufferUsageFlagBits::eTransferSrc;
  vkBufferCreateInfo.sharingMode          = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
  vmaAllocationCreateInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
  vmaAllocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VK_ASSERT_DEV(vmaCreateBuffer(m_pDeviceVulkan->GetVulkanMemoryAllocator(), reinterpret_cast<const VkBufferCreateInfo*>(&vkBufferCreateInfo), &vmaAllocationCreateInfo, reinterpret_cast<VkBuffer*>(&stagingBufferPage.m_vkBuffer), &stagingBufferPage.m_VmaAllocation, nullptr));

  stagingBufferPage.m_uiSize = uiSize;

  m_LargeAllocations.PushBack(stagingBufferPage);
}

xiiGALDynamicBufferAllocationVulkan xiiGALDynamicBufferPoolVulkan::Allocate(xiiUInt64 uiSize, bool bForceLargePage)
{
  if (bForceLargePage || uiSize >= (s_uiDynamicBufferDefaultPageSize >> 2))
  {
    CreateLargeBuffer(uiSize);

    const auto& largeAllocation = m_LargeAllocations.PeekBack();

    xiiGALDynamicBufferAllocationVulkan stagingBufferAllocation;
    stagingBufferAllocation.m_vkBuffer      = largeAllocation.m_vkBuffer;
    stagingBufferAllocation.m_VmaAllocation = largeAllocation.m_VmaAllocation;
    stagingBufferAllocation.m_uiOffset      = 0;

    return stagingBufferAllocation;
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

  // Sub allocate from current page.
  xiiGALDynamicBufferAllocationVulkan stagingBufferAllocation;
  stagingBufferAllocation.m_vkBuffer      = m_DynamicBufferPages[uiBufferID].m_vkBuffer;
  stagingBufferAllocation.m_VmaAllocation = m_DynamicBufferPages[uiBufferID].m_VmaAllocation;
  stagingBufferAllocation.m_uiOffset      = uiBufferAllocationOffset;

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return stagingBufferAllocation;
}

void xiiGALDynamicBufferPoolVulkan::Reset()
{
  m_uiPageAllocationCounter   = 0;
  m_uiOffsetAllocationCounter = 0;

  for (const auto& largeAllocation : m_LargeAllocations)
  {
    m_pDeviceVulkan->SafeReleaseDeviceObject(largeAllocation.m_vkBuffer, largeAllocation.m_VmaAllocation);
  }
  m_LargeAllocations.Clear();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_DynamicBufferPoolVulkan);
