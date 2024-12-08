#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

struct D3D12_RASTERIZER_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALRasterizerStateD3D12 final : public xiiGALRasterizerState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRasterizerStateD3D12, xiiGALRasterizerState);

public:
  XII_ALWAYS_INLINE const D3D12_RASTERIZER_DESC* GetRasterizerState() const { return &m_RasterizerState; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
  D3D12_RASTERIZER_DESC m_RasterizerState = {};
};
