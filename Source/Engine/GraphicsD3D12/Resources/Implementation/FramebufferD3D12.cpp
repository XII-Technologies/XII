#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

xiiGALFramebufferD3D12::xiiGALFramebufferD3D12(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(creationDescription)
{
}

xiiGALFramebufferD3D12::~xiiGALFramebufferD3D12() = default;

xiiResult xiiGALFramebufferD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::FramebufferDesc framebufferDescription;
  framebufferDescription.Name           = m_Description.m_sName.GetStartPointer();
  framebufferDescription.pRenderPass    = static_cast<xiiGALRenderPassD3D12*>(pDeviceD3D12->GetRenderPass(m_Description.m_hRenderPass))->GetRenderPass();
  framebufferDescription.Width          = m_Description.m_FramebufferSize.width;
  framebufferDescription.Height         = m_Description.m_FramebufferSize.height;
  framebufferDescription.NumArraySlices = m_Description.m_uiArraySliceCount;

  const xiiUInt32 uiAttachmentViewCount = m_Description.m_Attachments.GetCount();

  xiiHybridArray<Diligent::ITextureView*, 16U> attachmentViews;
  attachmentViews.SetCount(uiAttachmentViewCount);

  for (xiiUInt32 i = 0; i < uiAttachmentViewCount; ++i)
  {
    const auto& xiiAttachmentView = m_Description.m_Attachments[i];
    auto        attachmentView    = attachmentViews[i];

    attachmentView = static_cast<xiiGALTextureViewD3D12*>(pDeviceD3D12->GetTextureView(xiiAttachmentView))->GetTextureView();
  }
  framebufferDescription.AttachmentCount = uiAttachmentViewCount;
  framebufferDescription.ppAttachments   = attachmentViews.GetData();

  pDeviceD3D12->GetDevice()->CreateFramebuffer(framebufferDescription, &m_pFramebuffer);

  return (m_pFramebuffer != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALFramebufferD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pFramebuffer);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_FramebufferD3D12);
