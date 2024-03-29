#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSD3D11_DLL xiiGALTextureD3D11 final : public xiiGALTexture
{
public:
  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

  Diligent::ITexture* GetTexture() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALTextureD3D11(const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::ITexture* m_pTexture = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/TextureD3D11_inl.h>
