#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

struct D3D12_DEPTH_STENCIL_DESC;

class XII_GRAPHICSD3D12_DLL xiiGALDepthStencilStateD3D12 : public xiiGALDepthStencilState
{
public:
  XII_ALWAYS_INLINE const D3D12_DEPTH_STENCIL_DESC* GetDepthStencilState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateD3D12(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);

protected:
  D3D12_DEPTH_STENCIL_DESC m_DepthStencilState = {};
};

#include <GraphicsD3D12/States/Implementation/DepthStencilStateD3D12_inl.h>
