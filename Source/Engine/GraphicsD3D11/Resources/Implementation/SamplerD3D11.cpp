#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/SamplerD3D11.h>

xiiGALSamplerD3D11::xiiGALSamplerD3D11(const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(creationDescription)
{
}

xiiGALSamplerD3D11::~xiiGALSamplerD3D11() = default;

xiiResult xiiGALSamplerD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  D3D11_SAMPLER_DESC samplerDescription = {};
  samplerDescription.AddressU           = xiiD3D11TypeConversions::GetTextureAddressMode(m_Description.m_AddressU);
  samplerDescription.AddressV           = xiiD3D11TypeConversions::GetTextureAddressMode(m_Description.m_AddressV);
  samplerDescription.AddressW           = xiiD3D11TypeConversions::GetTextureAddressMode(m_Description.m_AddressW);
  samplerDescription.BorderColor[0]     = m_Description.m_BorderColor.r;
  samplerDescription.BorderColor[1]     = m_Description.m_BorderColor.g;
  samplerDescription.BorderColor[2]     = m_Description.m_BorderColor.b;
  samplerDescription.BorderColor[3]     = m_Description.m_BorderColor.a;
  samplerDescription.ComparisonFunc     = xiiD3D11TypeConversions::GetComparisonFunc(m_Description.m_ComparisonFunction);
  samplerDescription.MaxAnisotropy      = m_Description.m_uiMaxAnisotropy;
  samplerDescription.MinLOD             = m_Description.m_fMinLOD;
  samplerDescription.MaxLOD             = m_Description.m_fMaxLOD;
  samplerDescription.MipLODBias         = m_Description.m_fMipLODBias;

  if (m_Description.m_MagFilter == xiiGALFilterType::Anisotropic || m_Description.m_MinFilter == xiiGALFilterType::Anisotropic || m_Description.m_MipFilter == xiiGALFilterType::Anisotropic)
  {
    if (m_Description.m_ComparisonFunction == xiiGALComparisonFunction::Never)
    {
      samplerDescription.Filter = xiiD3D11TypeConversions::GetFilter(xiiGALFilterType::Anisotropic, xiiGALFilterType::Anisotropic, xiiGALFilterType::Anisotropic);
    }
    else
    {
      samplerDescription.Filter = xiiD3D11TypeConversions::GetFilter(xiiGALFilterType::ComparisonAnisotropic, xiiGALFilterType::ComparisonAnisotropic, xiiGALFilterType::ComparisonAnisotropic);
    }
  }
  else
  {
    samplerDescription.Filter = xiiD3D11TypeConversions::GetFilter(m_Description.m_MinFilter, m_Description.m_MagFilter, m_Description.m_MipFilter);
  }

  D3D11_DESCRIPTOR_HEAP_DESC samplerHeapDescription = {};
  samplerHeapDescription.Type                       = D3D11_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerHeapDescription.NumDescriptors             = 1U;
  samplerHeapDescription.Flags                      = D3D11_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  if (FAILED(pDeviceD3D11->GetDeviceD3D11()->CreateDescriptorHeap(&samplerHeapDescription, IID_PPV_ARGS(&m_pDescriptorHeap))))
  {
    xiiLog::Info("Failed to create descriptor heap for sampler {}.", GetDebugName());
    return XII_FAILURE;
  }

  pDeviceD3D11->GetDeviceD3D11()->CreateSampler(&samplerDescription, m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  return XII_SUCCESS;
}

xiiResult xiiGALSamplerD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  // Schedule deletion on device.
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_SamplerD3D11);
