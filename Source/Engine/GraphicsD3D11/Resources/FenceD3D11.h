#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

struct ID3D11Fence;

class XII_GRAPHICSD3D11_DLL xiiGALFenceD3D11 final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  ID3D11Fence* GetFence() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALFenceD3D11(const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  ID3D11Fence* m_pFence = nullptr;

  const HANDLE m_pFenceCompleteEvent;
};

#include <GraphicsD3D11/Resources/Implementation/FenceD3D11_inl.h>
