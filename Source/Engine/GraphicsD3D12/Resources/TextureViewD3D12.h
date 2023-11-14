#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSD3D12_DLL xiiGALTextureViewD3D12 final : public xiiGALTextureView
{
public:
  Diligent::ITextureView* GetTextureView() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTextureViewD3D12(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pTextureView;
};

#include <GraphicsD3D12/Resources/Implementation/TextureViewD3D12_inl.h>
