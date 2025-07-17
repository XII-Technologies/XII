#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class Buffer;
}

class XII_GRAPHICSVULKAN_DLL xiiGALDynamicBufferPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALDynamicBufferPoolVulkan);

public:
  struct DynamicBufferPage
  {
    vk::Buffer          m_vkBuffer;
    xiiVulkanAllocation m_VulkanAllocation;
    xiiUInt64           m_uiSize;
  };

  void CreateDynamicBufferPage();

  void CreateLargeBuffer(xiiUInt64 uiSize);

  xiiGALDynamicBufferAllocationVulkan Allocate(xiiUInt64 uiSize, bool bForceLargePage = false);

  void Reset();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALCommandQueueVulkan;
  friend class xiiGALCommandListVulkan;

  xiiGALDynamicBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags);
  ~xiiGALDynamicBufferPoolVulkan();

  static constexpr xiiUInt64 s_uiDynamicBufferDefaultPageSize = 1 * 1024 * 1024;

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiUInt32            m_uiAlignment;
  vk::BufferUsageFlags m_vkBufferUsageFlags;

  xiiDynamicArray<xiiGALDynamicBufferPoolVulkan::DynamicBufferPage> m_DynamicBufferPages;
  xiiDynamicArray<xiiGALDynamicBufferPoolVulkan::DynamicBufferPage> m_LargeAllocations;

  xiiUInt32 m_uiPageAllocationCounter   = 0U;
  xiiUInt64 m_uiOffsetAllocationCounter = 0U;
};
