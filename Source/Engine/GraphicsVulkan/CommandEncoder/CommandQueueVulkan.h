#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  virtual xiiUInt64 WaitForIdle() override final;

  Diligent::IDeviceContext* GetContext() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan& deviceVulkan, Diligent::IDeviceContext* pDeviceContext);

  virtual ~xiiGALCommandQueueVulkan();

protected:
  xiiGALDeviceVulkan& m_DeviceVulkan;

  Diligent::IDeviceContext* m_pContext = nullptr;

  Diligent::IFence* m_pFence                = nullptr;
  xiiUInt64         m_uiCompletedFenceValue = 0U;
};

#include <GraphicsVulkan/CommandEncoder/Implementation/CommandQueueVulkan_inl.h>
