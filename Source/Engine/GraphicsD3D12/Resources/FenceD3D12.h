#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Fence.h>

struct ID3D12Fence;

class XII_GRAPHICSD3D12_DLL xiiGALFenceD3D12 final : public xiiGALFence
{
public:
  virtual xiiUInt64 GetCompletedValue() override final;

  virtual void Signal(xiiUInt64 uiValue) override final;

  virtual void Wait(xiiUInt64 uiValue) override final;

  ID3D12Fence* GetFence() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALFenceD3D12(const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  ID3D12Fence* m_pFence = nullptr;

  const HANDLE m_pFenceCompleteEvent;
};

#include <GraphicsD3D12/Resources/Implementation/FenceD3D12_inl.h>
