#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>

xiiGALTextureViewVulkan::xiiGALTextureViewVulkan(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pTexture, creationDescription)
{
}

xiiGALTextureViewVulkan::~xiiGALTextureViewVulkan() = default;

xiiResult xiiGALTextureViewVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::TextureViewDesc viewDescription;
  viewDescription.ViewType        = xiiDiligentTypeConversions::GetTextureViewType(m_Description.m_ViewType);
  viewDescription.TextureDim      = xiiDiligentTypeConversions::GetResourceDimension(m_Description.m_ResourceDimension);
  viewDescription.Format          = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_Format);
  viewDescription.MostDetailedMip = m_Description.m_uiMostDetailedMip;
  viewDescription.NumMipLevels    = m_Description.m_uiMipLevelCount;
  viewDescription.AccessFlags     = xiiDiligentTypeConversions::GetUAVAccessFlags(m_Description.m_AccessFlags);
  viewDescription.Flags           = xiiDiligentTypeConversions::GetTextureViewFlags(m_Description.m_Flags);
  viewDescription.FirstArraySlice = m_Description.m_uiFirstArrayOrDepthSlice;
  viewDescription.FirstDepthSlice = m_Description.m_uiFirstArrayOrDepthSlice;
  viewDescription.NumArraySlices  = m_Description.m_uiArrayOrDepthSlicesCount;
  viewDescription.NumDepthSlices  = m_Description.m_uiArrayOrDepthSlicesCount;
  viewDescription.Swizzle.R       = xiiDiligentTypeConversions::GetComponentSwizzle(m_Description.m_ComponentSwizzle.m_R);
  viewDescription.Swizzle.G       = xiiDiligentTypeConversions::GetComponentSwizzle(m_Description.m_ComponentSwizzle.m_G);
  viewDescription.Swizzle.B       = xiiDiligentTypeConversions::GetComponentSwizzle(m_Description.m_ComponentSwizzle.m_B);
  viewDescription.Swizzle.A       = xiiDiligentTypeConversions::GetComponentSwizzle(m_Description.m_ComponentSwizzle.m_A);

  Diligent::ITexture* pTextureVulkan = const_cast<Diligent::ITexture*>(static_cast<xiiGALTextureVulkan*>(m_pTexture)->GetTexture());

  pTextureVulkan->CreateView(viewDescription, &m_pTextureView);

  return (m_pTextureView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALTextureViewVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureViewVulkan);
