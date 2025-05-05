#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSNULL_DLL xiiGALSamplerNull final : public xiiGALSampler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSamplerNull, xiiGALSampler);

public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALSamplerNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerNull();

  virtual xiiResult InitPlatform() override final;
};
