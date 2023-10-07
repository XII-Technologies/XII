#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>

// clang-format off
static const Diligent::TEXTURE_ADDRESS_MODE GALTextureAddressModeToD3D12[xiiGALTextureAddressMode::ENUM_COUNT] =
{
  Diligent::TEXTURE_ADDRESS_UNKNOWN,
  Diligent::TEXTURE_ADDRESS_WRAP,
  Diligent::TEXTURE_ADDRESS_MIRROR,
  Diligent::TEXTURE_ADDRESS_CLAMP,
  Diligent::TEXTURE_ADDRESS_BORDER,
  Diligent::TEXTURE_ADDRESS_MIRROR_ONCE,
};
// clang-format on

xiiGALSamplerD3D12::xiiGALSamplerD3D12(const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(creationDescription)
{
}

xiiGALSamplerD3D12::~xiiGALSamplerD3D12() = default;

xiiResult xiiGALSamplerD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  Diligent::SamplerDesc samplerDescription;
  samplerDescription.AddressU       = GALTextureAddressModeToD3D12[m_Description.m_AddressU];
  samplerDescription.AddressV       = GALTextureAddressModeToD3D12[m_Description.m_AddressV];
  samplerDescription.AddressW       = GALTextureAddressModeToD3D12[m_Description.m_AddressW];
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

  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);
  pDeviceD3D12->GetDevice()->CreateSampler(samplerDescription, &m_pSampler);

  return m_pSampler == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALSamplerD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pSampler);

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_SamplerD3D12);
