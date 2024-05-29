#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSamplerVulkan final : public xiiGALSampler
{
public:

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSamplerVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsVulkan/Resources/Implementation/SamplerVulkan_inl.h>
