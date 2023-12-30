#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

xiiGALTextureViewD3D12::xiiGALTextureViewD3D12(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pTexture, creationDescription)
{
}

xiiGALTextureViewD3D12::~xiiGALTextureViewD3D12() = default;

xiiResult xiiGALTextureViewD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::TextureViewDesc viewDescription;
  viewDescription.Name            = m_Description.m_sName.GetStartPointer();
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

  Diligent::ITexture* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(m_pTexture)->GetTexture();

  pTextureD3D12->CreateView(viewDescription, &m_pTextureView);

  return (m_pTextureView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALTextureViewD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureViewD3D12);
