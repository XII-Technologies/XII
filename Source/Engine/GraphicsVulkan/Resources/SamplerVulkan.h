#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSVULKAN_DLL xiiGALSamplerVulkan final : public xiiGALSampler
{
public:
  Diligent::ISampler* GetSampler() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALSamplerVulkan(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::ISampler* m_pSampler = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/SamplerVulkan_inl.h>
