
#pragma once

#include <RendererFoundation/State/State.h>


struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11RasterizerState2;
struct ID3D11SamplerState;

class XII_RENDERERDX11_DLL xiiGALBlendStateDX11 : public xiiGALBlendState
{
public:
  XII_ALWAYS_INLINE ID3D11BlendState* GetDXBlendState() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALBlendStateDX11(const xiiGALBlendStateCreationDescription& Description);

  ~xiiGALBlendStateDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11BlendState* m_pDXBlendState;
};

class XII_RENDERERDX11_DLL xiiGALDepthStencilStateDX11 : public xiiGALDepthStencilState
{
public:
  XII_ALWAYS_INLINE ID3D11DepthStencilState* GetDXDepthStencilState() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateDX11(const xiiGALDepthStencilStateCreationDescription& Description);

  ~xiiGALDepthStencilStateDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11DepthStencilState* m_pDXDepthStencilState;
};

class XII_RENDERERDX11_DLL xiiGALRasterizerStateDX11 : public xiiGALRasterizerState
{
public:
  XII_ALWAYS_INLINE ID3D11RasterizerState* GetDXRasterizerState() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateDX11(const xiiGALRasterizerStateCreationDescription& Description);

  ~xiiGALRasterizerStateDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11RasterizerState* m_pDXRasterizerState;
};

class XII_RENDERERDX11_DLL xiiGALSamplerStateDX11 : public xiiGALSamplerState
{
public:
  XII_ALWAYS_INLINE ID3D11SamplerState* GetDXSamplerState() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALSamplerStateDX11(const xiiGALSamplerStateCreationDescription& Description);

  ~xiiGALSamplerStateDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11SamplerState* m_pDXSamplerState;
};


#include <RendererDX11/State/Implementation/StateDX11_inl.h>
