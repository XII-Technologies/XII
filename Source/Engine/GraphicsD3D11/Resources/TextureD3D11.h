#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;

XII_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

class XII_GRAPHICSD3D11_DLL xiiGALTextureD3D11 final : public xiiGALTexture
{
public:
  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

  ID3D11Resource* GetTexture() const;
  ID3D11Resource* GetStagingTexture() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALTextureD3D11(const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  ID3D11Resource* m_pTexture        = nullptr;
  ID3D11Resource* m_pStagingTexture = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/TextureD3D11_inl.h>
