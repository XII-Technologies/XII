
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALTexture : public xiiGALResource<xiiGALTextureCreationDescription>
{
public:
protected:
  friend class xiiGALDevice;

  xiiGALTexture(const xiiGALTextureCreationDescription& Description);

  virtual ~xiiGALTexture();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};
