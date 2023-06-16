
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class xiiGALRenderTargetViewDX11 : public xiiGALRenderTargetView
{
public:
  XII_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  XII_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  XII_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALRenderTargetViewDX11(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description);

  virtual ~xiiGALRenderTargetViewDX11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView;

  ID3D11DepthStencilView* m_pDepthStencilView;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
