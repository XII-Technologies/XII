#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSNULL_DLL xiiGALTextureViewNull final : public xiiGALTextureView
{
public:
protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceNull;
  friend class xiiGALTextureNull;

  xiiGALTextureViewNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewNull();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;
};
