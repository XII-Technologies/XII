#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

struct ID3D11DescriptorHeap;

class XII_GRAPHICSD3D11_DLL xiiGALSamplerD3D11 final : public xiiGALSampler
{
public:
  D3D11_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D11(const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  ID3D11DescriptorHeap* m_pDescriptorHeap = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/SamplerD3D11_inl.h>
