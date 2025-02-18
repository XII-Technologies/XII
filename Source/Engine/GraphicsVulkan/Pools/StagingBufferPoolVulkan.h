#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

class XII_GRAPHICSVULKAN_DLL xiiGALStagingBufferPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALStagingBufferPoolVulkan);

public:
  struct StagingBufferPage
  {
    vk::Buffer    m_vkBuffer;
    VmaAllocation m_VmaAllocation;
    xiiUInt64     m_uiSize;
  };

  void CreateStagingBufferPage();

  void CreateLargeBuffer(xiiUInt64 uiSize);

  xiiGALStagingBufferAllocationVulkan Allocate(xiiUInt64 uiSize, bool bForceLargePage = false);

  void Reset();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  xiiGALStagingBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiAlignment, vk::BufferUsageFlags vkBufferUsageFlags);
  ~xiiGALStagingBufferPoolVulkan();

  static constexpr xiiUInt64 s_uiStagingBufferDefaultPageSize = 16 * 1024 * 1024;

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiUInt32            m_uiAlignment;
  vk::BufferUsageFlags m_vkBufferUsageFlags;

  xiiDynamicArray<xiiGALStagingBufferPoolVulkan::StagingBufferPage> m_StagingBufferPages;
  xiiDynamicArray<xiiGALStagingBufferPoolVulkan::StagingBufferPage> m_LargeAllocations;

  xiiUInt32 m_uiPageAllocationCounter   = 0U;
  xiiUInt64 m_uiOffsetAllocationCounter = 0U;
};
