#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

struct ID3D12DescriptorHeap;

class XII_GRAPHICSD3D12_DLL xiiGALSamplerD3D12 final : public xiiGALSampler
{
public:
  D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D12(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
};

#include <GraphicsD3D12/Resources/Implementation/SamplerD3D12_inl.h>
