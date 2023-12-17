#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSNULL_DLL xiiGALTextureNull final : public xiiGALTexture
{
public:
  virtual xiiGALTextureViewHandle GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType) override final;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALTextureNull(const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/Resources/Implementation/TextureNull_inl.h>
