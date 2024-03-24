#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSD3D12_DLL xiiGALSamplerD3D12 final : public xiiGALSampler
{
public:
  Diligent::ISampler* GetSampler() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D12(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::ISampler* m_pSampler = nullptr;
};

#include <GraphicsD3D12/Resources/Implementation/SamplerD3D12_inl.h>
