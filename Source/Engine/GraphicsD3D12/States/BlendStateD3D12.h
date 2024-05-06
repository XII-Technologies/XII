#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>

struct D3D12_BLEND_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALBlendStateD3D12 final : public xiiGALBlendState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBlendStateD3D12, xiiGALBlendState);

public:
  const D3D12_BLEND_DESC* GetBlendState() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBlendStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  D3D12_BLEND_DESC m_BlendState = {};
};

#include <GraphicsD3D12/States/Implementation/BlendStateD3D12_inl.h>
