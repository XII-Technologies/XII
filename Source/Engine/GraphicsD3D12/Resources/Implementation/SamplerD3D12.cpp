/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSamplerD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALSamplerD3D12::xiiGALSamplerD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALSamplerD3D12::~xiiGALSamplerD3D12() = default;

xiiResult xiiGALSamplerD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  D3D12_SAMPLER_DESC samplerDescription = {};
  samplerDescription.AddressU           = xiiD3D12TypeConversions::GetTextureAddressMode(m_Description.m_AddressU);
  samplerDescription.AddressV           = xiiD3D12TypeConversions::GetTextureAddressMode(m_Description.m_AddressV);
  samplerDescription.AddressW           = xiiD3D12TypeConversions::GetTextureAddressMode(m_Description.m_AddressW);
  samplerDescription.BorderColor[0]     = m_Description.m_BorderColor.r;
  samplerDescription.BorderColor[1]     = m_Description.m_BorderColor.g;
  samplerDescription.BorderColor[2]     = m_Description.m_BorderColor.b;
  samplerDescription.BorderColor[3]     = m_Description.m_BorderColor.a;
  samplerDescription.ComparisonFunc     = xiiD3D12TypeConversions::GetComparisonFunc(m_Description.m_ComparisonFunction);
  samplerDescription.MaxAnisotropy      = m_Description.m_uiMaxAnisotropy;
  samplerDescription.MinLOD             = m_Description.m_fMinLOD;
  samplerDescription.MaxLOD             = m_Description.m_fMaxLOD;
  samplerDescription.MipLODBias         = m_Description.m_fMipLODBias;

  if (m_Description.m_MagFilter == xiiGALFilterType::Anisotropic || m_Description.m_MinFilter == xiiGALFilterType::Anisotropic || m_Description.m_MipFilter == xiiGALFilterType::Anisotropic)
  {
    if (m_Description.m_ComparisonFunction == xiiGALComparisonFunction::Never)
    {
      samplerDescription.Filter = xiiD3D12TypeConversions::GetFilter(xiiGALFilterType::Anisotropic, xiiGALFilterType::Anisotropic, xiiGALFilterType::Anisotropic);
    }
    else
    {
      samplerDescription.Filter = xiiD3D12TypeConversions::GetFilter(xiiGALFilterType::ComparisonAnisotropic, xiiGALFilterType::ComparisonAnisotropic, xiiGALFilterType::ComparisonAnisotropic);
    }
  }
  else
  {
    samplerDescription.Filter = xiiD3D12TypeConversions::GetFilter(m_Description.m_MinFilter, m_Description.m_MagFilter, m_Description.m_MipFilter);
  }

  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDescription = {};
  samplerHeapDescription.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerHeapDescription.NumDescriptors             = 1U;
  samplerHeapDescription.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  if (FAILED(pDeviceD3D12->GetD3D12Device()->CreateDescriptorHeap(&samplerHeapDescription, IID_PPV_ARGS(&m_pDescriptorHeap))))
  {
    xiiLog::Info("Failed to create descriptor heap for sampler {}.", GetDebugName());
    return XII_FAILURE;
  }

  pDeviceD3D12->GetD3D12Device()->CreateSampler(&samplerDescription, m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_SamplerD3D12);
