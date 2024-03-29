#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

struct D3D11_RASTERIZER_DESC;

class XII_GRAPHICSD3D11_DLL xiiGALRasterizerStateD3D11 final : public xiiGALRasterizerState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRasterizerStateD3D11, xiiGALRasterizerState);

public:
  const D3D11_RASTERIZER_DESC* GetRasterizerState() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D11(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  D3D11_RASTERIZER_DESC m_RasterizerState = {};
};

#include <GraphicsD3D11/States/Implementation/RasterizerStateD3D11_inl.h>
