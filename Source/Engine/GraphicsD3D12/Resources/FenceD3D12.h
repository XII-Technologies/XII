/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  XII_ALWAYS_INLINE ID3D12Fence* GetD3D12Fence() const { return m_pD3D12Fence; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;

  xiiGALFenceD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALFenceCreationDescription& creationDescription);

  virtual ~xiiGALFenceD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

protected:
  ID3D12Fence* m_pD3D12Fence = nullptr;

  const HANDLE m_pFenceCompleteEvent;
};
