#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFramebufferVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALFramebufferVulkan::xiiGALFramebufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(std::move(pDeviceVulkan), creationDescription), m_vkFramebuffer(VK_NULL_HANDLE)
{
}

xiiGALFramebufferVulkan::~xiiGALFramebufferVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkFramebuffer));
}

xiiResult xiiGALFramebufferVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::FramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.pNext                     = nullptr;
  framebufferCreateInfo.flags                     = {};
  framebufferCreateInfo.width                     = m_Description.m_FramebufferSize.width;
  framebufferCreateInfo.height                    = m_Description.m_FramebufferSize.height;
  framebufferCreateInfo.layers                    = m_Description.m_uiArraySliceCount;

  xiiSharedPtr<xiiGALRenderPassVulkan> pRenderPassVulkan = m_Description.m_pRenderPass.Downcast<xiiGALRenderPassVulkan>();
  framebufferCreateInfo.renderPass                       = pRenderPassVulkan->GetVulkanRenderPass();

  xiiHybridArray<vk::ImageView, 8U> vkImageViews(pDeviceVulkan->GetAllocator());
  for (xiiUInt32 i = 0; i < m_Description.m_Attachments.GetCount(); ++i)
  {
    xiiSharedPtr<xiiGALTextureViewVulkan> pAttachmentView = m_Description.m_Attachments[i].Downcast<xiiGALTextureViewVulkan>();

    if (pAttachmentView != nullptr)
    {
      vkImageViews.PushBack(pAttachmentView->GetVulkanImageView());
    }
    else
    {
      vkImageViews.PushBack(vk::ImageView());
    }
  }
  framebufferCreateInfo.attachmentCount = vkImageViews.GetCount();
  framebufferCreateInfo.pAttachments    = vkImageViews.GetData();

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &m_vkFramebuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

void xiiGALFramebufferVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkFramebuffer, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FramebufferVulkan);
