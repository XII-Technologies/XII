#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandQueueD3D12 final : public xiiGALCommandQueue
{
public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  virtual xiiUInt64 WaitForIdle() override final;

  Diligent::IDeviceContext* GetContext() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D12(xiiGALDeviceD3D12& deviceD3D12, Diligent::IDeviceContext* pDeviceContext);

  virtual ~xiiGALCommandQueueD3D12();

protected:
  xiiGALDeviceD3D12& m_DeviceD3D12;

  Diligent::IDeviceContext* m_pContext = nullptr;

  Diligent::IFence* m_pFence                = nullptr;
  xiiUInt64         m_uiCompletedFenceValue = 0U;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandQueueD3D12_inl.h>
