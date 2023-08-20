#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

struct D3D12_RASTERIZER_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALRasterizerStateD3D12 : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE const D3D12_RASTERIZER_DESC* GetRasterizerState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D12(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  D3D12_RASTERIZER_DESC m_RasterizerState = {};
};

#include <GraphicsD3D12/States/Implementation/RasterizerStateD3D12_inl.h>
