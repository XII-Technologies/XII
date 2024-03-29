#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSD3D11_DLL xiiGALCommandQueueD3D11 final : public xiiGALCommandQueue
{
public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  virtual xiiUInt64 WaitForIdle() override final;

  Diligent::IDeviceContext* GetContext() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D11(xiiGALDeviceD3D11& deviceD3D11, Diligent::IDeviceContext* pDeviceContext);

  virtual ~xiiGALCommandQueueD3D11();

protected:
  xiiGALDeviceD3D11& m_DeviceD3D11;

  Diligent::IDeviceContext* m_pContext = nullptr;

  Diligent::IFence* m_pFence                = nullptr;
  xiiUInt64         m_uiCompletedFenceValue = 0U;
};

#include <GraphicsD3D11/CommandEncoder/Implementation/CommandQueueD3D11_inl.h>
