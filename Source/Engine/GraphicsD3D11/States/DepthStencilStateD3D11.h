#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

struct D3D11_DEPTH_STENCIL_DESC;

class XII_GRAPHICSD3D11_DLL xiiGALDepthStencilStateD3D11 final : public xiiGALDepthStencilState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDepthStencilStateD3D11, xiiGALDepthStencilState);

public:
  const D3D11_DEPTH_STENCIL_DESC* GetDepthStencilState() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateD3D11(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  D3D11_DEPTH_STENCIL_DESC m_DepthStencilState = {};
};

#include <GraphicsD3D11/States/Implementation/DepthStencilStateD3D11_inl.h>
