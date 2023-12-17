#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSNULL_DLL xiiGALSamplerNull final : public xiiGALSampler
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALSamplerNull(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/Resources/Implementation/SamplerNull_inl.h>
