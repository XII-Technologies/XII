#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/FramebufferD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>

xiiGALFramebufferD3D11::xiiGALFramebufferD3D11(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(creationDescription)
{
}

xiiGALFramebufferD3D11::~xiiGALFramebufferD3D11() = default;

xiiResult xiiGALFramebufferD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::FramebufferDesc framebufferDescription;
  framebufferDescription.Name           = m_Description.m_sName.GetStartPointer();
  framebufferDescription.pRenderPass    = static_cast<xiiGALRenderPassD3D11*>(pDeviceD3D11->GetRenderPass(m_Description.m_hRenderPass))->GetRenderPass();
  framebufferDescription.Width          = m_Description.m_FramebufferSize.width;
  framebufferDescription.Height         = m_Description.m_FramebufferSize.height;
  framebufferDescription.NumArraySlices = m_Description.m_uiArraySliceCount;

  const xiiUInt32 uiAttachmentViewCount = m_Description.m_Attachments.GetCount();

  xiiHybridArray<Diligent::ITextureView*, 16U> attachmentViews;
  attachmentViews.SetCount(uiAttachmentViewCount);

  for (xiiUInt32 i = 0; i < uiAttachmentViewCount; ++i)
  {
    const auto& xiiAttachmentView = m_Description.m_Attachments[i];

    attachmentViews[i] = static_cast<xiiGALTextureViewD3D11*>(pDeviceD3D11->GetTextureView(xiiAttachmentView))->GetTextureView();
  }
  framebufferDescription.AttachmentCount = uiAttachmentViewCount;
  framebufferDescription.ppAttachments   = attachmentViews.GetData();

  pDeviceD3D11->GetDevice()->CreateFramebuffer(framebufferDescription, &m_pFramebuffer);

  // Resolve frame buffer size if none was provided in the creation description.
  if (m_pFramebuffer && !m_Description.m_FramebufferSize.HasNonZeroArea())
  {
    const auto& description = m_pFramebuffer->GetDesc();

    m_Description.m_FramebufferSize.width  = description.Width;
    m_Description.m_FramebufferSize.height = description.Height;
    m_Description.m_uiArraySliceCount      = description.NumArraySlices;
  }

  return (m_pFramebuffer != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALFramebufferD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFramebuffer);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_FramebufferD3D11);
