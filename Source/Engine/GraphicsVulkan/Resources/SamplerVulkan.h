#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

namespace vk
{
  class Sampler;
}

class XII_GRAPHICSVULKAN_DLL xiiGALSamplerVulkan final : public xiiGALSampler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSamplerVulkan, xiiGALSampler);

public:
  XII_ALWAYS_INLINE vk::Sampler GetVulkanSampler() const { return m_vkSampler; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSamplerVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::Sampler m_vkSampler;
};
