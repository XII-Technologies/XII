#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>

xiiGALSamplerVulkan::xiiGALSamplerVulkan(const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(creationDescription)
{
}

xiiGALSamplerVulkan::~xiiGALSamplerVulkan() = default;

xiiResult xiiGALSamplerVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::SamplerDesc samplerDescription;
  samplerDescription.AddressU       = xiiDiligentTypeConversions::GetTextureAddress(m_Description.m_AddressU);
  samplerDescription.AddressV       = xiiDiligentTypeConversions::GetTextureAddress(m_Description.m_AddressV);
  samplerDescription.AddressW       = xiiDiligentTypeConversions::GetTextureAddress(m_Description.m_AddressW);
  samplerDescription.BorderColor[0] = m_Description.m_BorderColor.r;
  samplerDescription.BorderColor[1] = m_Description.m_BorderColor.g;
  samplerDescription.BorderColor[2] = m_Description.m_BorderColor.b;
  samplerDescription.BorderColor[3] = m_Description.m_BorderColor.a;
  samplerDescription.ComparisonFunc = xiiDiligentTypeConversions::GetComparisonFunc(m_Description.m_ComparisonFunction);

  if (m_Description.m_MagFilter == xiiGALFilterType::Anisotropic || m_Description.m_MinFilter == xiiGALFilterType::Anisotropic || m_Description.m_MipFilter == xiiGALFilterType::Anisotropic)
  {
    if (m_Description.m_ComparisonFunction == xiiGALComparisonFunction::Never)
    {
      samplerDescription.MinFilter = samplerDescription.MagFilter = samplerDescription.MipFilter = Diligent::FILTER_TYPE_ANISOTROPIC;
    }
    else
    {
      samplerDescription.MinFilter = samplerDescription.MagFilter = samplerDescription.MipFilter = Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC;
    }
  }
  else
  {
    samplerDescription.MinFilter = xiiDiligentTypeConversions::GetFilter(m_Description.m_MinFilter);
    samplerDescription.MagFilter = xiiDiligentTypeConversions::GetFilter(m_Description.m_MagFilter);
    samplerDescription.MipFilter = xiiDiligentTypeConversions::GetFilter(m_Description.m_MipFilter);
  }

  samplerDescription.MaxAnisotropy = m_Description.m_uiMaxAnisotropy;
  samplerDescription.MinLOD        = m_Description.m_fMinLOD;
  samplerDescription.MaxLOD        = m_Description.m_fMaxLOD;
  samplerDescription.MipLODBias    = m_Description.m_fMipLODBias;

  pDeviceVulkan->GetDevice()->CreateSampler(samplerDescription, &m_pSampler);

  return m_pSampler == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALSamplerVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pSampler);

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_SamplerVulkan);
