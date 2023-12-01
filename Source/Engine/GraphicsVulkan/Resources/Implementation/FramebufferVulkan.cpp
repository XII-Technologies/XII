#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALFramebufferVulkan::xiiGALFramebufferVulkan(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(creationDescription)
{
}

xiiGALFramebufferVulkan::~xiiGALFramebufferVulkan() = default;

xiiResult xiiGALFramebufferVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::FramebufferDesc framebufferDescription;
  framebufferDescription.Name           = m_Description.m_sName.GetStartPointer();
  framebufferDescription.pRenderPass    = static_cast<xiiGALRenderPassVulkan*>(pDeviceVulkan->GetRenderPass(m_Description.m_hRenderPass))->GetRenderPass();
  framebufferDescription.Width          = m_Description.m_FramebufferSize.width;
  framebufferDescription.Height         = m_Description.m_FramebufferSize.height;
  framebufferDescription.NumArraySlices = m_Description.m_uiArraySliceCount;

  const xiiUInt32 uiAttachmentViewCount = m_Description.m_Attachments.GetCount();

  xiiHybridArray<Diligent::ITextureView*, 16U> attachmentViews;
  attachmentViews.SetCount(uiAttachmentViewCount);

  for (xiiUInt32 i = 0; i < uiAttachmentViewCount; ++i)
  {
    const auto& xiiAttachmentView = m_Description.m_Attachments[i];

    attachmentViews[i] = static_cast<xiiGALTextureViewVulkan*>(pDeviceVulkan->GetTextureView(xiiAttachmentView))->GetTextureView();
  }
  framebufferDescription.AttachmentCount = uiAttachmentViewCount;
  framebufferDescription.ppAttachments   = attachmentViews.GetData();

  pDeviceVulkan->GetDevice()->CreateFramebuffer(framebufferDescription, &m_pFramebuffer);

  // Resolve frame buffer size if none was provided in the creation description.
  if (m_pFramebuffer && !m_Description.m_FramebufferSize.HasNonZeroArea())
  {
    const auto& description = m_pFramebuffer->GetDesc();

    m_Description.m_FramebufferSize.width  = description.Width;
    m_Description.m_FramebufferSize.height = description.Height;
  }

  return (m_pFramebuffer != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALFramebufferVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFramebuffer);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FramebufferVulkan);
