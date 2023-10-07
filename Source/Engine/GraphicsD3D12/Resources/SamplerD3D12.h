#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSD3D12_DLL xiiGALSamplerD3D12 final : public xiiGALSampler
{
public:
  XII_ALWAYS_INLINE const Diligent::ISampler* GetSampler() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D12(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ISampler> m_pSampler;
};

#include <GraphicsD3D12/Resources/Implementation/SamplerD3D12_inl.h>
