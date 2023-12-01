#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSD3D12_DLL xiiGALTextureD3D12 final : public xiiGALTexture
{
public:
  virtual xiiGALTextureViewHandle GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType) override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override;

  Diligent::ITexture* GetTexture() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTextureD3D12(const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::ITexture* m_pTexture = nullptr;
};

#include <GraphicsD3D12/Resources/Implementation/TextureD3D12_inl.h>
