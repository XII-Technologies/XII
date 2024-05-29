#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() override final;

  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& description);

  virtual ~xiiGALCommandQueueVulkan();

protected:
  xiiUInt64 m_uiCompletedFenceValue = 0U;
};

#include <GraphicsVulkan/CommandEncoder/Implementation/CommandQueueVulkan_inl.h>
