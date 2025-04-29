#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSNULL_DLL xiiGALTextureNull final : public xiiGALTexture
{
public:
  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALTextureNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureNull();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiInternal::NewInstance<xiiGALTextureView> CreateViewPlatform(const xiiGALTextureViewCreationDescription& description) override;
};
