#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFramebufferVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALFramebufferVulkan::xiiGALFramebufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(pDeviceVulkan, creationDescription)
{
}

xiiGALFramebufferVulkan::~xiiGALFramebufferVulkan() = default;

xiiResult xiiGALFramebufferVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::FramebufferCreateInfo framebufferCreateInfo = {};
  framebufferCreateInfo.pNext                     = nullptr;
  framebufferCreateInfo.flags                     = {};
  framebufferCreateInfo.width                     = m_Description.m_FramebufferSize.width;
  framebufferCreateInfo.height                    = m_Description.m_FramebufferSize.height;
  framebufferCreateInfo.layers                    = m_Description.m_uiArraySliceCount;

  xiiGALRenderPassVulkan* pRenderPassVulkan = static_cast<xiiGALRenderPassVulkan*>(pDeviceVulkan->GetRenderPass(m_Description.m_hRenderPass));
  framebufferCreateInfo.renderPass          = pRenderPassVulkan->GetVulkanRenderPass();

  xiiHybridArray<vk::ImageView, 8U> vkImageViews(pDeviceVulkan->GetAllocator());
  for (xiiUInt32 i = 0; i < m_Description.m_Attachments.GetCount(); ++i)
  {
    const xiiGALTextureViewHandle& hAttachmentView = m_Description.m_Attachments[i];

    if (!hAttachmentView.IsInvalidated())
    {
      xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pDeviceVulkan->GetTextureView(m_Description.m_Attachments[i]));

      vkImageViews.PushBack(pTextureViewVulkan->GetVulkanImageView());
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

xiiResult xiiGALFramebufferVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(m_vkFramebuffer);

  m_vkFramebuffer = VK_NULL_HANDLE;

  return XII_SUCCESS;
}

void xiiGALFramebufferVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkFramebuffer, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FramebufferVulkan);
