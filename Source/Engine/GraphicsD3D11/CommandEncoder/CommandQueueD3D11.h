#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSD3D11_DLL xiiGALCommandQueueD3D11 final : public xiiGALCommandQueue
{
public:
  virtual xiiUInt64 GetNextFenceValue() const override final;

  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11);

  virtual ~xiiGALCommandQueueD3D11();

protected:
  xiiUInt64 m_uiCompletedFenceValue = 0U;
};

#include <GraphicsD3D11/CommandEncoder/Implementation/CommandQueueD3D11_inl.h>
