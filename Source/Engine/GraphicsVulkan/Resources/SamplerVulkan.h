#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSamplerVulkan final : public xiiGALSampler
{
public:
  XII_ALWAYS_INLINE const Diligent::ISampler* GetSampler() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSamplerVulkan(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ISampler> m_pSampler;
};

#include <GraphicsVulkan/Resources/Implementation/SamplerVulkan_inl.h>
