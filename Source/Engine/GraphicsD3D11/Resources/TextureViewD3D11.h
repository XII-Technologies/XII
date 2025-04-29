#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

struct ID3D11View;

class XII_GRAPHICSD3D11_DLL xiiGALTextureViewD3D11 final : public xiiGALTextureView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureViewD3D11, xiiGALTextureView);

public:
  XII_ALWAYS_INLINE ID3D11View* GetTextureView() const { return m_pTextureView; };

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D11;
  friend class xiiGALTextureD3D11;

  xiiGALTextureViewD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  xiiResult CreateSRV(ID3D11ShaderResourceView** ppShaderResourceView);
  xiiResult CreateRTV(ID3D11RenderTargetView** ppRenderTargetView);
  xiiResult CreateDSV(ID3D11DepthStencilView** ppDepthStencilView);
  xiiResult CreateUAV(ID3D11UnorderedAccessView** ppUnorderedAccessView);

  ID3D11View* m_pTextureView = nullptr;
};
