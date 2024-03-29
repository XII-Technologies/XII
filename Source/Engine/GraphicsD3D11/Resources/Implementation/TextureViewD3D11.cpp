#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>

xiiGALTextureViewD3D11::xiiGALTextureViewD3D11(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pTexture, creationDescription)
{
}

xiiGALTextureViewD3D11::~xiiGALTextureViewD3D11() = default;

xiiResult xiiGALTextureViewD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

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

  Diligent::ITexture* pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(m_pTexture)->GetTexture();

  pTextureD3D11->CreateView(viewDescription, &m_pTextureView);

  return (m_pTextureView != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALTextureViewD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_D3D11_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TextureViewD3D11);
