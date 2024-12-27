#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

#define XII_VERIFY_COMMAND_LIST(expression, ...) \
  do                                             \
  {                                              \
    XII_ASSERT_DEV((expression), __VA_ARGS__);   \
    if (!(expression)) { return; }               \
  } while (false)

#define XII_VERIFY_COMMAND_LIST_RESULT(expression, ...) \
  do                                                    \
  {                                                     \
    XII_ASSERT_DEV((expression), __VA_ARGS__);          \
    if (!(expression)) { return XII_FAILURE; }          \
  } while (false)


// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALStateTransitionFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::UpdateState),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::DiscardContent),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::Aliasing),
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

[[nodiscard]] vk::AccessFlags AccessFlagsFromImageLayout(vk::ImageLayout vkImageLayout, bool bIsDestinationMask)
{
  vk::AccessFlags vkAccessFlags = {};

  switch (vkImageLayout)
  {
    // does not support device access. This layout must only be used as the initialLayout member of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
    // When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved (11.4)
    case vk::ImageLayout::eUndefined:
      if (bIsDestinationMask)
      {
        XII_ASSERT_DEV(false, "The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED. "
                              "This layout must only be used as the initialLayout member of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
      }
      break;

    // supports all types of device access
    case vk::ImageLayout::eGeneral:
      // VK_IMAGE_LAYOUT_GENERAL must be used for image load/store operations (13.1.1, 13.2.4)
      vkAccessFlags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
      break;

    // must only be used as a color or resolve attachment in a VkFramebuffer (11.4)
    case vk::ImageLayout::eColorAttachmentOptimal:
      vkAccessFlags = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
      break;

    // must only be used as a depth/stencil attachment in a VkFramebuffer (11.4)
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      vkAccessFlags = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      break;

    // must only be used as a read-only depth/stencil attachment in a VkFramebuffer and/or as a read-only image in a shader (11.4)
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
      vkAccessFlags = vk::AccessFlagBits::eDepthStencilAttachmentRead;
      break;

    // must only be used as a read-only image in a shader (which can be read as a sampled image,
    // combined image/sampler and/or input attachment) (11.4)
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      vkAccessFlags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
      break;

    //  must only be used as a source image of a transfer command (11.4)
    case vk::ImageLayout::eTransferSrcOptimal:
      vkAccessFlags = vk::AccessFlagBits::eTransferRead;
      break;

    // must only be used as a destination image of a transfer command (11.4)
    case vk::ImageLayout::eTransferDstOptimal:
      vkAccessFlags = vk::AccessFlagBits::eTransferWrite;
      break;

    // does not support device access. This layout must only be used as the initialLayout member
    // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
    // When transitioning out of this layout, the contents of the memory are preserved. (11.4)
    case vk::ImageLayout::ePreinitialized:
      if (!bIsDestinationMask)
      {
        vkAccessFlags = vk::AccessFlagBits::eHostWrite;
      }
      else
      {
        XII_ASSERT_DEV(false, "The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED. "
                              "This layout must only be used as the initialLayout member of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
      }
      break;

    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
      vkAccessFlags = vk::AccessFlagBits::eDepthStencilAttachmentRead;
      break;

    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
      vkAccessFlags = vk::AccessFlagBits::eDepthStencilAttachmentRead;
      break;

    // When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    // there is no need to delay subsequent processing, or perform any visibility operations (as vkQueuePresentKHR
    // performs automatic visibility operations). To achieve this, the dstvkAccessFlags member of the VkImageMemoryBarrier
    // should be set to 0, and the dstStageMask parameter should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
    case vk::ImageLayout::ePresentSrcKHR:
      vkAccessFlags = {};
      break;

    case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
      vkAccessFlags = vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
      break;

    case vk::ImageLayout::eFragmentDensityMapOptimalEXT:
      vkAccessFlags = vk::AccessFlagBits::eFragmentDensityMapReadEXT;
      break;

    default:
      XII_ASSERT_DEV(false, "Unexpected image layout.");
      break;
  }

  return vkAccessFlags;
}

template <bool AllowTouch, typename T>
bool CheckLineSectionOverlap(T min0, T max0, T min1, T max1)
{
  XII_ASSERT_DEV(min0 <= max0 && min1 <= max1, "");
  //     [------]         [------]
  //   min0    max0    min1     max1
  //
  //     [------]         [------]
  //   min1    max1    min0     max0
  if (AllowTouch)
  {
    return !(min0 > max1 || min1 > max0);
  }
  else
  {
    return !(min0 >= max1 || min1 >= max0);
  }
}

void xiiGALCommandListVulkan::TransitionImageLayout(vk::Image vkImage, vk::ImageLayout vkOldLayout, vk::ImageLayout vkNewLayout, const vk::ImageSubresourceRange& vkImageSubresourceRange, vk::PipelineStageFlags vkPipelineSourceStageFlags, vk::PipelineStageFlags vkPipelineDestinationStageFlags)
{
  // Should we end render pass automatically?
  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

  XII_VERIFY_COMMAND_LIST((vkPipelineSourceStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags), "");
  XII_VERIFY_COMMAND_LIST((vkPipelineDestinationStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags), "");

  if (vkOldLayout == vkNewLayout)
  {
    m_PipelineBarrier.m_vkMemorySourceStages |= vkPipelineSourceStageFlags;
    m_PipelineBarrier.m_vkMemoryDestinationStages |= vkPipelineDestinationStageFlags;

    m_PipelineBarrier.m_vkMemorySourceAccess |= AccessFlagsFromImageLayout(vkOldLayout, false);
    m_PipelineBarrier.m_vkMemoryDestinationAccess |= AccessFlagsFromImageLayout(vkNewLayout, true);

    return;
  }

  // Check for overlapping subresources.
  for (xiiUInt32 i = 0; i < m_ImageBarriers.GetCount(); ++i)
  {
    const auto& vkImageMemoryBarrier = m_ImageBarriers[i];
    if (vkImageMemoryBarrier.image != vkImage)
      continue;

    const auto& vkOtherRange = vkImageMemoryBarrier.subresourceRange;

    const xiiUInt32 uiStartLayer0 = vkImageSubresourceRange.baseArrayLayer;
    const xiiUInt32 uiEndLayer0   = vkImageSubresourceRange.layerCount != vk::RemainingArrayLayers ? (vkImageSubresourceRange.baseArrayLayer + vkImageSubresourceRange.layerCount) : ~0U;
    const xiiUInt32 uiStartLayer1 = vkOtherRange.baseArrayLayer;
    const xiiUInt32 uiEndLayer1   = vkImageSubresourceRange.layerCount != vk::RemainingArrayLayers ? (vkOtherRange.baseArrayLayer + vkOtherRange.layerCount) : ~0U;

    const xiiUInt32 uiStartMipLevel0 = vkImageSubresourceRange.baseMipLevel;
    const xiiUInt32 uiEndMipLevel0   = vkImageSubresourceRange.levelCount != vk::RemainingMipLevels ? (vkImageSubresourceRange.baseMipLevel + vkImageSubresourceRange.levelCount) : ~0U;
    const xiiUInt32 uiStartMipLevel1 = vkOtherRange.baseMipLevel;
    const xiiUInt32 uiEndMipLevel1   = vkImageSubresourceRange.levelCount != vk::RemainingMipLevels ? (vkOtherRange.baseMipLevel + vkOtherRange.levelCount) : ~0U;

    const bool bSlicesOverlap = CheckLineSectionOverlap<true>(uiStartLayer0, uiEndLayer0, uiStartLayer1, uiEndLayer1);
    const bool bMipsOverlap   = CheckLineSectionOverlap<true>(uiStartMipLevel0, uiEndMipLevel0, uiStartMipLevel1, uiEndMipLevel1);

    // If the range overlaps with any of the existing barriers, we need to flush them.
    if (bSlicesOverlap && bMipsOverlap)
    {
      FlushBarriers();
      break;
    }
  }

  m_PipelineBarrier.m_vkImageSourceStages |= vkPipelineSourceStageFlags;
  m_PipelineBarrier.m_vkImageDestinationStages |= vkPipelineDestinationStageFlags;

  vk::ImageMemoryBarrier vkImageMemoryBarrier = {};
  vkImageMemoryBarrier.pNext                  = nullptr;
  vkImageMemoryBarrier.oldLayout              = vkOldLayout;
  vkImageMemoryBarrier.newLayout              = vkNewLayout;
  vkImageMemoryBarrier.image                  = vkImage;
  vkImageMemoryBarrier.subresourceRange       = vkImageSubresourceRange;
  vkImageMemoryBarrier.srcAccessMask          = AccessFlagsFromImageLayout(vkOldLayout, false) & m_PipelineBarrier.m_vkSupportedAccessFlags;
  vkImageMemoryBarrier.dstAccessMask          = AccessFlagsFromImageLayout(vkNewLayout, true) & m_PipelineBarrier.m_vkSupportedAccessFlags;
  vkImageMemoryBarrier.srcQueueFamilyIndex    = vk::QueueFamilyIgnored; // Source queue family for a queue family ownership transfer.
  vkImageMemoryBarrier.dstQueueFamilyIndex    = vk::QueueFamilyIgnored; // Destination queue family for a queue family ownership transfer.

  m_ImageBarriers.PushBack(vkImageMemoryBarrier);
}

void xiiGALCommandListVulkan::MemoryBarrier(vk::AccessFlags vkSourceAccessFlags, vk::AccessFlags vkDestinationAccessFlags, vk::PipelineStageFlags vkPipelineSourceStageFlags, vk::PipelineStageFlags vkPipelineDestinationStageFlags)
{
  // Should we end render pass automatically?
  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

  XII_VERIFY_COMMAND_LIST((vkPipelineSourceStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags), "");
  XII_VERIFY_COMMAND_LIST((vkPipelineDestinationStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags), "");

  m_PipelineBarrier.m_vkMemorySourceStages |= vkPipelineSourceStageFlags;
  m_PipelineBarrier.m_vkMemoryDestinationStages |= vkPipelineDestinationStageFlags;

  m_PipelineBarrier.m_vkMemorySourceAccess |= vkSourceAccessFlags;
  m_PipelineBarrier.m_vkMemoryDestinationAccess |= vkDestinationAccessFlags;
}

void xiiGALCommandListVulkan::FlushBarriers()
{
  if (m_PipelineBarrier.m_vkMemorySourceStages == vk::PipelineStageFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationStages == vk::PipelineStageFlagBits::eNone && m_ImageBarriers.IsEmpty())
    return;

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
  {
    EndRenderPassPlatform();
  }

  vk::MemoryBarrier vkMemoryBarrier = {};
  vkMemoryBarrier.pNext             = nullptr;
  vkMemoryBarrier.srcAccessMask     = m_PipelineBarrier.m_vkMemorySourceAccess & m_PipelineBarrier.m_vkSupportedAccessFlags;
  vkMemoryBarrier.dstAccessMask     = m_PipelineBarrier.m_vkMemoryDestinationAccess & m_PipelineBarrier.m_vkSupportedAccessFlags;

  const bool bHasMemoryBarrier = m_PipelineBarrier.m_vkMemorySourceStages != vk::PipelineStageFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationStages == vk::PipelineStageFlagBits::eNone &&
    m_PipelineBarrier.m_vkMemorySourceAccess == vk::AccessFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationAccess == vk::AccessFlagBits::eNone;

  const vk::PipelineStageFlags vkSourceStages      = (m_PipelineBarrier.m_vkImageSourceStages | m_PipelineBarrier.m_vkMemorySourceStages) & m_PipelineBarrier.m_vkSupportedStageFlags;
  const vk::PipelineStageFlags vkDestinationStages = (m_PipelineBarrier.m_vkImageDestinationStages | m_PipelineBarrier.m_vkMemoryDestinationStages) & m_PipelineBarrier.m_vkSupportedStageFlags;

  XII_VERIFY_COMMAND_LIST(vkSourceStages != vk::PipelineStageFlagBits::eNone && vkDestinationStages != vk::PipelineStageFlagBits::eNone, "");

  m_vkCommandBuffer.pipelineBarrier(vkSourceStages, vkDestinationStages, vk::DependencyFlagBits::eByRegion, bHasMemoryBarrier ? 1U : 0U, bHasMemoryBarrier ? &vkMemoryBarrier : nullptr, 0, nullptr, m_ImageBarriers.GetCount(), m_ImageBarriers.IsEmpty() ? nullptr : m_ImageBarriers.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_ImageBarriers.Clear();

  m_PipelineBarrier.m_vkImageSourceStages       = {};
  m_PipelineBarrier.m_vkImageDestinationStages  = {};
  m_PipelineBarrier.m_vkMemorySourceStages      = {};
  m_PipelineBarrier.m_vkMemoryDestinationStages = {};
  m_PipelineBarrier.m_vkMemorySourceAccess      = {};
  m_PipelineBarrier.m_vkMemoryDestinationAccess = {};

  // Do not clear SupportedStagesMask and SupportedAccessMask.
}

void xiiGALCommandListVulkan::CopyBufferToImage(vk::Buffer vkSourceBuffer, vk::Image vkDestinationImage, vk::ImageLayout vkDestinationImageLayout, xiiArrayPtr<const vk::BufferImageCopy> pRegions)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  // Copy operations must be performed outside of render pass.
  if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
  {
    EndRenderPassPlatform();
  }

  FlushBarriers();

  m_vkCommandBuffer.copyBufferToImage(vkSourceBuffer, vkDestinationImage, vkDestinationImageLayout, pRegions.GetCount(), pRegions.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyImageToBuffer(vk::Image vkSourceImage, vk::ImageLayout vkSourceImageLayout, vk::Buffer vkDestinationBuffer, xiiArrayPtr<const vk::BufferImageCopy> pRegions)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  // Copy operations must be performed outside of render pass.
  if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
  {
    EndRenderPassPlatform();
  }

  FlushBarriers();

  m_vkCommandBuffer.copyImageToBuffer(vkSourceImage, vkSourceImageLayout, vkDestinationBuffer, pRegions.GetCount(), pRegions.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags pipelineFlags)
{
  XII_ASSERT_DEV(semaphore != VK_NULL_HANDLE, "");

  m_vkWaitSemaphores.PushBack(semaphore);
  m_vkWaitDestinationStageFlags.PushBack(pipelineFlags);
  m_vkWaitSemaphoreValues.PushBack(0); // Ignored for binary semaphore.
}

void xiiGALCommandListVulkan::AddSignalSemaphore(vk::Semaphore semaphore)
{
  XII_ASSERT_DEV(semaphore != VK_NULL_HANDLE, "");

  m_vkSignalSemaphores.PushBack(semaphore);
  m_vkSignalSemaphoreValues.PushBack(0); // Ignored for binary semaphore.
}

xiiGALCommandListVulkan::xiiGALCommandListVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceVulkan, pCommandQueueVulkan, creationDescription)
{
  vk::CommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
  vkCommandBufferAllocateInfo.pNext                         = nullptr;
  vkCommandBufferAllocateInfo.commandPool                   = pCommandQueueVulkan->GetVulkanCommandPool();
  vkCommandBufferAllocateInfo.level                         = vk::CommandBufferLevel::ePrimary;
  vkCommandBufferAllocateInfo.commandBufferCount            = 1U;

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanLogicalDevice().allocateCommandBuffers(&vkCommandBufferAllocateInfo, &m_vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
}

xiiGALCommandListVulkan::~xiiGALCommandListVulkan()
{
  xiiGALDeviceVulkan*       pDeviceVulkan       = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(m_pCommandQueue);

  pDeviceVulkan->GetVulkanLogicalDevice().freeCommandBuffers(pCommandQueueVulkan->GetVulkanCommandPool(), 1U, &m_vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::BeginPlatform()
{
  xiiGALDeviceVulkan*       pDeviceVulkan       = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(m_pCommandQueue);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  m_PipelineBarrier.m_vkSupportedStageFlags  = pCommandQueueVulkan->GetSupportedStagesFlags();
  m_PipelineBarrier.m_vkSupportedAccessFlags = pCommandQueueVulkan->GetSupportedAccessFlags();

  vk::CommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.pNext                      = nullptr;
  vkCommandBufferBeginInfo.flags                      = vk::CommandBufferUsageFlagBits::eOneTimeSubmit; // Each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission.
  vkCommandBufferBeginInfo.pInheritanceInfo           = nullptr;                                        // Ignored for a primary command buffer.

  VK_ASSERT_DEV(m_vkCommandBuffer.begin(&vkCommandBufferBeginInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_RecordingState = RecordingState::Recording;

  pCommandQueueVulkan->BeginCommandList(this);
}

void xiiGALCommandListVulkan::EndPlatform()
{
  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  FlushBarriers();

  m_vkCommandBuffer.end(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_RecordingState = RecordingState::Ended;
}

void xiiGALCommandListVulkan::ResetPlatform()
{
  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(GetCommandQueue());
  pCommandQueueVulkan->ResetCommandList(this);
}

void xiiGALCommandListVulkan::ResetInternal()
{
  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  InvalidateState();

  m_RecordingState = RecordingState::Reset;
}

xiiUInt64 xiiGALCommandListVulkan::SubmitPlatform(bool bReset)
{
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(GetCommandQueue());
  return pCommandQueueVulkan->SubmitCommandList(this, bReset);
}

void xiiGALCommandListVulkan::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
}

void xiiGALCommandListVulkan::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, uiStencilRef, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.setBlendConstants(blendFactor.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_VERIFY_COMMAND_LIST(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::Viewport vkViewPorts[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiViewPortIndex = 0; uiViewPortIndex < pViewports.GetCount(); ++uiViewPortIndex)
  {
    vkViewPorts[uiViewPortIndex].x        = pViewports[uiViewPortIndex].m_fTopLeftX;
    vkViewPorts[uiViewPortIndex].y        = pViewports[uiViewPortIndex].m_fTopLeftY;
    vkViewPorts[uiViewPortIndex].width    = pViewports[uiViewPortIndex].m_fWidth;
    vkViewPorts[uiViewPortIndex].height   = pViewports[uiViewPortIndex].m_fHeight;
    vkViewPorts[uiViewPortIndex].minDepth = pViewports[uiViewPortIndex].m_fMinDepth;
    vkViewPorts[uiViewPortIndex].maxDepth = pViewports[uiViewPortIndex].m_fMaxDepth;

    // Turn the viewport upside down to be consistent with Direct3D. Note that in both APIs, the viewport covers the same texture rows. The difference is that Direct3D inverts
    // normalized device Y coordinate when transforming NDC to window coordinates. In Vulkan, we achieve the same effect by using negative viewport height. Therefore we need to
    // invert normalized device Y coordinate when transforming to texture V.
    //
    //
    //       Image                Direct3D                                       Image               Vulkan
    //        row                                                                 row
    //         0 _   (0,0)_______________________(1,0)                  Tex Height _   (0,1)_______________________(1,1)
    //         1 _       |                       |      |             VP Top + Hght _ _ _ _|   __________          |      A
    //         2 _       |                       |      |                          .       |  |   .--> +x|         |      |
    //           .       |                       |      |                          .       |  |   |      |         |      |
    //           .       |                       |      | V Coord                          |  |   V +y   |         |      | V Coord
    //     VP Top _ _ _ _|   __________          |      |                    VP Top _ _ _ _|  |__________|         |      |
    //           .       |  |    A +y  |         |      |                          .       |                       |      |
    //           .       |  |    |     |         |      |                          .       |                       |      |
    //           .       |  |    '-->+x|         |      |                        2 _       |                       |      |
    //           .       |  |__________|         |      |                        1 _       |                       |      |
    //Tex Height _       |_______________________|      V                        0 _       |_______________________|      |
    //               (0,1)                       (1,1)                                 (0,0)                       (1,0)
    //
    //

    vkViewPorts[uiViewPortIndex].y      = vkViewPorts[uiViewPortIndex].y + vkViewPorts[uiViewPortIndex].height;
    vkViewPorts[uiViewPortIndex].height = -vkViewPorts[uiViewPortIndex].height;
  }

  m_vkCommandBuffer.setViewport(0, m_Viewports.GetCount(), vkViewPorts, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_VERIFY_COMMAND_LIST(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::Rect2D vkScissorRects[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiScissorRectIndex = 0; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    vkScissorRects[uiScissorRectIndex].offset = vk::Offset2D{static_cast<xiiInt32>(pRects[uiScissorRectIndex].x), static_cast<xiiInt32>(pRects[uiScissorRectIndex].y)};
    vkScissorRects[uiScissorRectIndex].extent = vk::Extent2D{pRects[uiScissorRectIndex].width, pRects[uiScissorRectIndex].height};
  }

  m_vkCommandBuffer.setScissor(0, m_ScissorRects.GetCount(), vkScissorRects, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
}

void xiiGALCommandListVulkan::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
}

[[nodiscard]] vk::ClearColorValue ClearValueToVulkanClearValue(const void* pClearValues, xiiGALResourceFormat::Enum textureFormat)
{
  vk::ClearColorValue vkClearValue     = {};
  const auto&         formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureFormat);

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::SignedInteger)
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      vkClearValue.int32[i] = static_cast<const xiiInt32*>(pClearValues)[i];
    }
  }
  else if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::UnsignedInteger)
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      vkClearValue.uint32[i] = static_cast<const xiiUInt32*>(pClearValues)[i];
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      vkClearValue.float32[i] = static_cast<const float*>(pClearValues)[i];
    }
  }

  return vkClearValue;
}

void xiiGALCommandListVulkan::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  xiiGALDeviceVulkan*      pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pRenderTargetView);
  xiiGALTextureVulkan*     pTextureVulkan     = static_cast<xiiGALTextureVulkan*>(pRenderTargetView->GetTexture());
  const auto&              viewDescription    = pRenderTargetView->GetDescription();

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  // Check if the texture is one of the currently bound render targets.
  xiiUInt32 uiAttachmentIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; ++i)
  {
    if (m_pBoundRenderTargets[i] == pRenderTargetView)
    {
      uiAttachmentIndex = i;
      break;
    }
  }

  XII_VERIFY_COMMAND_LIST(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE || uiAttachmentIndex != xiiInvalidIndex, "Render target was not found in the frame buffer. This is unexpected because the render pass should either be invalid or the render target view should be part of an active render pass.");

  if (uiAttachmentIndex != xiiInvalidIndex)
  {
    XII_VERIFY_COMMAND_LIST(m_pRenderPass != nullptr && m_pFramebuffer != nullptr, "The render pass or frame buffer is invalid while the texture was bound.");

    // The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT(17.1)

    vk::ClearAttachment vkClearAttachment = {};
    vkClearAttachment.aspectMask          = vk::ImageAspectFlagBits::eColor;
    vkClearAttachment.clearValue.color    = ClearValueToVulkanClearValue(clearColor.GetData(), viewDescription.m_Format);

    // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT is set in aspectMask, in which case it is an index to the pColorAttachments array in the VkSubpassDescription
    // structure of the current subpass which selects the color attachment to clear (17.2).
    // It is NOT the render pass attachment index.
    vkClearAttachment.colorAttachment = uiAttachmentIndex;

    vk::ClearRect vkClearRect  = {};
    vkClearRect.rect           = vk::Rect2D{{0, 0}, {m_uiFramebufferWidth, m_uiFramebufferHeight}}; // m_uiFramebufferWidth, m_uiFramebufferHeight are scaled to the proper mip level.
    vkClearRect.baseArrayLayer = 0;                                                                 // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice.
    vkClearRect.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;

    // No memory barriers are needed between vkCmdClearAttachments and preceding or subsequent draw or attachment clear commands in the same subpass (17.2)
    m_vkCommandBuffer.clearAttachments(1U, &vkClearAttachment, 1U, &vkClearRect, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    XII_VERIFY_COMMAND_LIST(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdClearColorImage() must be called outside render pass (17.1)");

    // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
    TransitionOrVerifyTextureState(pTextureVulkan, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Clearing render target outside of render pass.");

    // The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT(17.1)

    vk::ImageSubresourceRange vkImageSubresourceRange = {};
    vkImageSubresourceRange.aspectMask                = vk::ImageAspectFlagBits::eColor;
    vkImageSubresourceRange.baseMipLevel              = viewDescription.m_uiMostDetailedMip;
    vkImageSubresourceRange.levelCount                = viewDescription.m_uiMipLevelCount;
    vkImageSubresourceRange.baseArrayLayer            = viewDescription.m_uiFirstArrayOrDepthSlice;
    vkImageSubresourceRange.layerCount                = viewDescription.m_uiArrayOrDepthSlicesCount;

    XII_ASSERT_DEV(viewDescription.m_uiMipLevelCount > 0, "Render target view must contain at least a single mip level.");

    vk::ClearColorValue vkClearColorValue = ClearValueToVulkanClearValue(clearColor.GetData(), viewDescription.m_Format);

    FlushBarriers();

    // Must either be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
    m_vkCommandBuffer.clearColorImage(pTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, &vkClearColorValue, 1U, &vkImageSubresourceRange, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  xiiGALDeviceVulkan*      pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pDepthStencilView);
  xiiGALTextureVulkan*     pTextureVulkan     = static_cast<xiiGALTextureVulkan*>(pDepthStencilView->GetTexture());
  const auto&              viewDescription    = pDepthStencilView->GetDescription();

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST(viewDescription.m_ResourceDimension != xiiGALResourceDimension::Texture3D, "Depth-stencil view of a 3D texture must be created as a 2D texture array view.");

  const bool bClearAsAttachment = m_pBoundDepthStencilTarget == pTextureViewVulkan;
  XII_VERIFY_COMMAND_LIST(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE || bClearAsAttachment, "Depth-stencil view was not found in the frame buffer. This is unexpected because the render pass should either be invalid or the depth stencil view should be part of an active render pass.");

  if (bClearAsAttachment)
  {
    XII_VERIFY_COMMAND_LIST(m_pRenderPass != nullptr && m_pFramebuffer != nullptr, "The render pass or frame buffer is invalid while the texture was bound.");

    // The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT(17.1)

    vk::ClearAttachment vkClearAttachment = {};
    vkClearAttachment.aspectMask          = {};

    if (bClearDepth)
      vkClearAttachment.aspectMask |= vk::ImageAspectFlagBits::eDepth;
    if (bClearStencil)
      vkClearAttachment.aspectMask |= vk::ImageAspectFlagBits::eStencil;

    vkClearAttachment.colorAttachment                 = vk::AttachmentUnused; // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT is set in aspectMask
    vkClearAttachment.clearValue.depthStencil.depth   = fDepthClear;
    vkClearAttachment.clearValue.depthStencil.stencil = uiStencilClear;

    vk::ClearRect vkClearRect  = {};
    vkClearRect.rect           = vk::Rect2D{{0, 0}, {m_uiFramebufferWidth, m_uiFramebufferHeight}}; // m_uiFramebufferWidth, m_uiFramebufferHeight are scaled to the proper mip level.
    vkClearRect.baseArrayLayer = 0;                                                                 // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice.
    vkClearRect.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;

    // No memory barriers are needed between vkCmdClearAttachments and preceding or subsequent draw or attachment clear commands in the same subpass (17.2)
    m_vkCommandBuffer.clearAttachments(1U, &vkClearAttachment, 1U, &vkClearRect, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    XII_VERIFY_COMMAND_LIST(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdClearDepthStencilImage() must be called outside render pass (17.1)");

    // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
    TransitionOrVerifyTextureState(pTextureVulkan, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Clearing depth-stencil outside of render pass.");

    // The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT(17.1)

    vk::ImageSubresourceRange vkImageSubresourceRange = {};
    vkImageSubresourceRange.aspectMask                = {};
    vkImageSubresourceRange.baseMipLevel              = viewDescription.m_uiMostDetailedMip;
    vkImageSubresourceRange.levelCount                = viewDescription.m_uiMipLevelCount;
    vkImageSubresourceRange.baseArrayLayer            = viewDescription.m_uiFirstArrayOrDepthSlice;
    vkImageSubresourceRange.layerCount                = viewDescription.m_uiArrayOrDepthSlicesCount;

    if (bClearDepth)
      vkImageSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eDepth;
    if (bClearStencil)
      vkImageSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;

    vk::ClearDepthStencilValue vkClearDepthStencilValue = {};
    vkClearDepthStencilValue.depth                      = fDepthClear;
    vkClearDepthStencilValue.stencil                    = uiStencilClear;

    FlushBarriers();

    // Must either be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
    m_vkCommandBuffer.clearDepthStencilImage(pTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, &vkClearDepthStencilValue, 1U, &vkImageSubresourceRange, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
}

void xiiGALCommandListVulkan::NextSubpassPlatform()
{
}

void xiiGALCommandListVulkan::EndRenderPassPlatform()
{
}

xiiResult xiiGALCommandListVulkan::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDraw() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  m_vkCommandBuffer.draw(uiVertexCount, 1U, uiStartVertex, 0, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexed() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkIndexBuffer != VK_NULL_HANDLE, "No index buffer bound.");

  m_vkCommandBuffer.drawIndexed(uiIndexCount, 1U, uiStartIndex, uiBaseVertex, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexed() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkIndexBuffer != VK_NULL_HANDLE, "No index buffer bound.");

  m_vkCommandBuffer.drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexedindirect() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkIndexBuffer != VK_NULL_HANDLE, "No index buffer bound.");

  m_vkCommandBuffer.drawIndexedIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, 0U, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDraw() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  m_vkCommandBuffer.draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndirect() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  m_vkCommandBuffer.drawIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, 0U, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawMeshTasksEXT() must be called inside render pass (19.3)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  m_vkCommandBuffer.drawMeshTasksEXT(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdDispatch() must be called outside of render pass (27)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound.");

  FlushBarriers();

  m_vkCommandBuffer.dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  XII_VERIFY_COMMAND_LIST_RESULT(m_vkCommandBuffer != VK_NULL_HANDLE, "");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdDispatchIndirect() must be called outside of render pass (27)");
  XII_VERIFY_COMMAND_LIST_RESULT(m_CommandListState.m_vkComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound.");

  FlushBarriers();

  m_vkCommandBuffer.dispatchIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListVulkan::EndQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListVulkan::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
}

void xiiGALCommandListVulkan::UpdateBufferExtendedPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags, bool bCopyToTemporaryStorage)
{
}

void xiiGALCommandListVulkan::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
}

void xiiGALCommandListVulkan::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
}

xiiResult xiiGALCommandListVulkan::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListVulkan::UpdateTextureExtendedPlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListVulkan::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
}

void xiiGALCommandListVulkan::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
}

void xiiGALCommandListVulkan::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
}

void xiiGALCommandListVulkan::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
}

xiiResult xiiGALCommandListVulkan::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiStringBuilder tmp;

  vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
  vkDebugUtilsLabel.pNext                  = nullptr;
  vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
  vkDebugUtilsLabel.color[0]               = color.r;
  vkDebugUtilsLabel.color[1]               = color.g;
  vkDebugUtilsLabel.color[2]               = color.b;
  vkDebugUtilsLabel.color[3]               = color.a;

  m_vkCommandBuffer.beginDebugUtilsLabelEXT(vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::EndDebugGroupPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  m_vkCommandBuffer.endDebugUtilsLabelEXT(pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiStringBuilder tmp;

  vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
  vkDebugUtilsLabel.pNext                  = nullptr;
  vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
  vkDebugUtilsLabel.color[0]               = color.r;
  vkDebugUtilsLabel.color[1]               = color.g;
  vkDebugUtilsLabel.color[2]               = color.b;
  vkDebugUtilsLabel.color[3]               = color.a;

  m_vkCommandBuffer.insertDebugUtilsLabelEXT(vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::FlushPlatform()
{
}

void xiiGALCommandListVulkan::InvalidateStatePlatform()
{
  m_CommandListState = {};
  m_PipelineBarrier  = {};

  m_ImageBarriers.Clear();

  m_vkWaitSemaphores.Clear();
  m_vkSignalSemaphores.Clear();
  m_vkWaitDestinationStageFlags.Clear();
  m_vkWaitSemaphoreValues.Clear();
  m_vkSignalSemaphoreValues.Clear();
  m_SignalFences.Clear();
  m_WaitFences.Clear();
}

void xiiGALCommandListVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandBuffer, sName.GetData(tmp));
}

[[nodiscard]] inline bool ResourceStateHasWriteAccess(xiiBitflags<xiiGALResourceStateFlags> flags)
{
  xiiBitflags<xiiGALResourceStateFlags> writeAccessStates = xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::BuildAsWrite;

  return writeAccessStates.IsAnySet(flags);
}

void xiiGALCommandListVulkan::TransitionBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, const bool bUpdateBufferState)
{
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

  if (oldState == xiiGALResourceStateFlags::Unknown)
  {
    if (pBufferVulkan->IsInKnownState())
    {
      oldState = pBufferVulkan->GetResourceState();
    }
    else
    {
      xiiLog::Error("Failed to execute buffer memory barrier for buffer '{}' because the buffer state is unknown and is not explicitly specified.", pBufferVulkan->GetDebugName());
      return;
    }
  }
  else
  {
    if (pBufferVulkan->IsInKnownState() && pBufferVulkan->GetResourceState() != oldState)
    {
      xiiLog::Error("The state ({}) of buffer '{}' does not match the old state ({}) specified by the barrier.", pBufferVulkan->GetResourceState().GetValue(), pBufferVulkan->GetDebugName(), oldState.GetValue());
    }
  }

  // Always add barrier after writes.
  const bool bAfterWrite = ResourceStateHasWriteAccess(oldState);

  if (((oldState & newState) != newState) || bAfterWrite)
  {
    XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

    auto oldAccessFlags = xiiVulkanTypeConversions::GetAccessFlags(oldState);
    auto newAccessFlags = xiiVulkanTypeConversions::GetAccessFlags(newState);
    auto oldStages      = xiiVulkanTypeConversions::GetPipelineStageFlags(oldState);
    auto newStages      = xiiVulkanTypeConversions::GetPipelineStageFlags(newState);

    MemoryBarrier(oldAccessFlags, newAccessFlags, oldStages, newStages);

    if (bUpdateBufferState)
    {
      pBufferVulkan->SetResourceState(newState);
    }
  }
}

void xiiGALCommandListVulkan::BufferMemoryBarrier(xiiGALBufferVulkan* pBufferVulkan, vk::AccessFlags newAccessFlags)
{
  XII_VERIFY_COMMAND_LIST(pBufferVulkan != nullptr, "");

  if (!pBufferVulkan->IsInKnownState())
  {
    xiiLog::Error("Failed to execute buffer memory barrier for buffer '{}' because the buffer state is unknown.", pBufferVulkan->GetDebugName());
    return;
  }

  xiiBitflags<xiiGALResourceStateFlags> newState = xiiVulkanTypeConversions::GetResourceState(newAccessFlags);

  if ((pBufferVulkan->GetResourceState() & newState) != newState)
  {
    TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, newState, true);
  }
}

void xiiGALCommandListVulkan::TransitionTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, xiiBitflags<xiiGALStateTransitionFlags> flags, vk::ImageSubresourceRange* pSubresourceRange)
{
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

  if (oldState == xiiGALResourceStateFlags::Unknown)
  {
    if (pTextureVulkan->IsInKnownState())
    {
      oldState = pTextureVulkan->GetResourceState();
    }
    else
    {
      xiiLog::Error("Failed to transition the state of texture '{}' because the state is unknown and is not explicitly specified.", pTextureVulkan->GetDebugName());
      return;
    }
  }
  else
  {
    if (pTextureVulkan->IsInKnownState() && pTextureVulkan->GetResourceState() != oldState)
    {
      xiiLog::Error("The state ({}) of texture '{}' does not match the old state ({}) specified by the barrier.", pTextureVulkan->GetResourceState().GetValue(), pTextureVulkan->GetDebugName(), oldState.GetValue());
    }
  }

  XII_VERIFY_COMMAND_LIST(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  vk::ImageSubresourceRange vkImageFullSubresourceRange = {};
  if (pSubresourceRange == nullptr)
  {
    pSubresourceRange = &vkImageFullSubresourceRange;

    vkImageFullSubresourceRange.aspectMask     = vk::ImageAspectFlagBits::eNone;
    vkImageFullSubresourceRange.baseArrayLayer = 0;
    vkImageFullSubresourceRange.layerCount     = vk::RemainingArrayLayers;
    vkImageFullSubresourceRange.baseMipLevel   = 0;
    vkImageFullSubresourceRange.levelCount     = vk::RemainingMipLevels;
  }

  if (pSubresourceRange->aspectMask == vk::ImageAspectFlagBits::eNone)
  {
    const auto& textureDescription = pTextureVulkan->GetDescription();
    const auto& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
    {
      pSubresourceRange->aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      // If image has a depth / stencil format with both depth and stencil components, then the aspectMask member of subresourceRange must include both VK_IMAGE_ASPECT_DEPTH_BIT and VK_IMAGE_ASPECT_STENCIL_BIT (6.7.3)
      pSubresourceRange->aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }
    else
    {
      pSubresourceRange->aspectMask = vk::ImageAspectFlagBits::eColor;
    }
  }

  // Always add barrier after writes.
  const bool bAfterWrite = ResourceStateHasWriteAccess(oldState);

  xiiGALDeviceVulkan* pDeviceVulkan       = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  const auto&         extensionFeatures   = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();
  const bool          bFragmentDensityMap = extensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False;
  const auto          oldLayout           = flags.IsSet(xiiGALStateTransitionFlags::DiscardContent) ? vk::ImageLayout::eUndefined : xiiVulkanTypeConversions::GetImageLayout(oldState, false, bFragmentDensityMap);
  const auto          newLayout           = xiiVulkanTypeConversions::GetImageLayout(newState, false, bFragmentDensityMap);
  const auto          oldStages           = xiiVulkanTypeConversions::GetPipelineStageFlags(oldState);
  const auto          newStages           = xiiVulkanTypeConversions::GetPipelineStageFlags(newState);

  if (((oldState & newState) != newState) || oldLayout != newLayout || bAfterWrite)
  {
    TransitionImageLayout(pTextureVulkan->GetVulkanImage(), oldLayout, newLayout, *pSubresourceRange, oldStages, newStages);

    if (flags.IsSet(xiiGALStateTransitionFlags::UpdateState))
    {
      pTextureVulkan->SetResourceState(newState);

      XII_ASSERT_DEV(pTextureVulkan->GetVulkanImageLayout() == newLayout, "");
    }
  }
}

void xiiGALCommandListVulkan::TransitionImageLayout(xiiGALTextureVulkan* pTextureVulkan, vk::ImageLayout newLayout)
{
  XII_VERIFY_COMMAND_LIST(pTextureVulkan != nullptr, "");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

  if (!pTextureVulkan->IsInKnownState())
  {
    xiiLog::Error("Failed to transition layout for texture '{}' because the texture state is unknown", pTextureVulkan->GetDebugName());
    return;
  }

  auto newState = (xiiGALResourceStateFlags::Enum)xiiVulkanTypeConversions::GetResourceState(newLayout).GetValue();
  if (!pTextureVulkan->CheckState(newState))
  {
    TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, newState, xiiGALStateTransitionFlags::UpdateState);
  }
}

void xiiGALCommandListVulkan::TransitionOrVerifyBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::AccessFlagBits expectedAccessFlags, const char* szOperationName, bool bVerifyOnly)
{
  if (!bVerifyOnly)
  {
    XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

    if (pBufferVulkan->IsInKnownState())
    {
      TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, requiredState, true);

      XII_ASSERT_DEV(pBufferVulkan->CheckAccessFlags(expectedAccessFlags), "");
    }
  }
  else
  {
    // TODO
  }
}

void xiiGALCommandListVulkan::TransitionOrVerifyTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::ImageLayout expectedLayout, const char* szOperationName, bool bVerifyOnly)
{
  if (!bVerifyOnly)
  {
    XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "State transitions are not permitted while a render pass is active.");

    if (pTextureVulkan->IsInKnownState())
    {
      TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, requiredState, xiiGALStateTransitionFlags::UpdateState);

      XII_ASSERT_DEV(pTextureVulkan->GetVulkanImageLayout() == expectedLayout, "");
    }
  }
  else
  {
    // TODO
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandListVulkan);
