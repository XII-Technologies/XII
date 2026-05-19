/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class XII_GRAPHICSD3D12_DLL xiiGALSamplerD3D12 final : public xiiGALSampler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSamplerD3D12, xiiGALSampler);

public:
  XII_ALWAYS_INLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return m_CPUDescriptorHandle; }
  XII_ALWAYS_INLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const { return m_GPUDescriptorHandle; }
  XII_ALWAYS_INLINE ID3D12DescriptorHeap*       GetDescriptorHeap() const { return m_pDescriptorHeap; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALSamplerD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALSamplerCreationDescription& creationDescription);

  virtual ~xiiGALSamplerD3D12();

  virtual xiiResult InitPlatform() override final;

protected:
  ID3D12DescriptorHeap*       m_pDescriptorHeap    = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle = {};
};
