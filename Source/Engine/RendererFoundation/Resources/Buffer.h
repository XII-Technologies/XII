
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class XII_RENDERERFOUNDATION_DLL xiiGALBuffer : public xiiGALResource<xiiGALBufferCreationDescription>
{
public:
  XII_ALWAYS_INLINE xiiUInt32 GetSize() const;

protected:
  friend class xiiGALDevice;

  xiiGALBuffer(const xiiGALBufferCreationDescription& Description);

  virtual ~xiiGALBuffer();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<const xiiUInt8> pInitialData) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>
