/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>
#include <GraphicsVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <GraphicsVulkan/Pools/QueryPoolVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>
#include <GraphicsVulkan/States/ComputePipelineStateVulkan.h>
#include <GraphicsVulkan/States/GraphicsPipelineStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/RayTracingPipelineStateVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandListVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] XII_FORCE_INLINE vk::QueryPool CreateCompactedSizeQueryPool(const xiiGALDeviceVulkan* pDeviceVulkan)
  {
    vk::QueryPoolCreateInfo vkQueryPoolCreateInfo = {};
    vkQueryPoolCreateInfo.queryType               = vk::QueryType::eAccelerationStructureCompactedSizeKHR;
    vkQueryPoolCreateInfo.queryCount              = 1U;

    vk::QueryPool vkQueryPool     = VK_NULL_HANDLE;
    vk::Device    vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    VK_ASSERT_DEV(vkLogicalDevice.createQueryPool(&vkQueryPoolCreateInfo, nullptr, &vkQueryPool, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    return vkQueryPool;
  }

  [[nodiscard]] XII_ALWAYS_INLINE vk::BuildAccelerationStructureFlagsKHR ConvertBuildASFlags(xiiBitflags<xiiGALRayTracingBuildASFlags> flags)
  {
    vk::BuildAccelerationStructureFlagsKHR vkFlags = {};

    if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowUpdate))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::AllowCompaction))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastTrace))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::PreferFastBuild))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild;
    if (flags.IsSet(xiiGALRayTracingBuildASFlags::LowMemory))
      vkFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eLowMemory;

    return vkFlags;
  }

  [[nodiscard]] XII_FORCE_INLINE vk::Format ConvertTriangleVertexFormat(const xiiGALBLASTriangleDescription& triangle)
  {
    if (triangle.m_VertexValueType == xiiGALValueType::Float32)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32Sfloat;
    }

    if (triangle.m_VertexValueType == xiiGALValueType::Float16)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR16G16Sfloat : vk::Format::eR16G16B16Sfloat;
    }

    if (triangle.m_VertexValueType == xiiGALValueType::Int32)
    {
      return triangle.m_uiVertexComponentCount == 2U ? vk::Format::eR32G32Sint : vk::Format::eR32G32B32Sint;
    }

    return vk::Format::eUndefined;
  }

  [[nodiscard]] XII_FORCE_INLINE vk::ClearColorValue ClearValueToVulkanClearValue(const void* pClearValues, xiiGALResourceFormat::Enum textureFormat)
  {
    vk::ClearColorValue                    vkClearValue     = {};
    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureFormat);

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

  [[nodiscard]] vk::AccessFlags AccessFlagsFromImageLayout(vk::ImageLayout vkImageLayout, bool bIsDestinationMask)
  {
    vk::AccessFlags vkAccessFlags = {};

    switch (vkImageLayout)
    {
      // does not support device access. This layout must only be used as the initialLayout member of vk::ImageCreateInfo or vk::AttachmentDescription, or as the oldLayout in an image transition.
      // When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved (11.4)
      case vk::ImageLayout::eUndefined:
        if (bIsDestinationMask)
        {
          XII_REPORT_FAILURE("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED. This layout must only be used as the initialLayout member of vk::ImageCreateInfo or vk::AttachmentDescription, or as the oldLayout in an image transition. (11.4)");
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

      // must only be used as a read-only image in a shader (which can be read as a sampled image, combined image/sampler and/or input attachment) (11.4)
      case vk::ImageLayout::eShaderReadOnlyOptimal:
        vkAccessFlags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
        break;

      // must only be used as a source image of a transfer command (11.4)
      case vk::ImageLayout::eTransferSrcOptimal:
        vkAccessFlags = vk::AccessFlagBits::eTransferRead;
        break;

      // must only be used as a destination image of a transfer command (11.4)
      case vk::ImageLayout::eTransferDstOptimal:
        vkAccessFlags = vk::AccessFlagBits::eTransferWrite;
        break;

      // does not support device access. This layout must only be used as the initialLayout member of vk::ImageCreateInfo or vk::AttachmentDescription, or as the oldLayout in an image transition.
      // When transitioning out of this layout, the contents of the memory are preserved. (11.4)
      case vk::ImageLayout::ePreinitialized:
        if (!bIsDestinationMask)
        {
          vkAccessFlags = vk::AccessFlagBits::eHostWrite;
        }
        else
        {
          XII_REPORT_FAILURE("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED. This layout must only be used as the initialLayout member of vk::ImageCreateInfo or vk::AttachmentDescription, or as the oldLayout in an image transition. (11.4)");
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
        XII_REPORT_FAILURE("Unexpected image layout.");
        break;
    }

    return vkAccessFlags;
  }

  /// Checks whether two 1D line segments overlap.
  ///
  /// This function determines if the intervals [min0, max0] and [min1, max1] overlap.
  /// The behavior depends on the template parameter `AllowTouch`:
  /// - If `AllowTouch` is true, touching endpoints are considered overlapping.
  /// - If `AllowTouch` is false, touching endpoints are not considered overlapping.
  ///
  /// \tparam AllowTouch - If true, segments that touch at endpoints are considered overlapping.
  /// \tparam T          - A numeric type (e.g., float, double, int) used for the segment bounds.
  ///
  /// \param min0 - Lower bound of the first segment.
  /// \param max0 - Upper bound of the first segment.
  /// \param min1 - Lower bound of the second segment.
  /// \param max1 - Upper bound of the second segment.
  ///
  /// \return True if the segments overlap (or touch, depending on AllowTouch); false otherwise.
  ///
  /// \note The function assumes that min0 <= max0 and min1 <= max1.
  template <bool AllowTouch, typename T>
  bool CheckLineSectionOverlap(T min0, T max0, T min1, T max1)
  {
    // Ensure valid intervals: min must not exceed max for either segment
    XII_ASSERT_DEV(min0 <= max0 && min1 <= max1, "Invalid segment bounds: min must be <= max");
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

  [[nodiscard]] vk::BufferImageCopy GetBufferImageCopyInfo(xiiUInt64 uiBufferOffset, xiiUInt32 uiBufferRowStrideInTexels, const xiiGALTextureCreationDescription& textureDescription, const xiiBoundingBoxU32& region, xiiUInt32 uiMipLevel, xiiUInt32 uiArraySlice)
  {
    vk::BufferImageCopy vkBufferImageCopyRegion = {};

    XII_ASSERT_DEV((uiBufferOffset % 4) == 0, "Source buffer offset must be multiple of 4 (18.4)");
    vkBufferImageCopyRegion.bufferOffset = uiBufferOffset; // must be a multiple of 4 (18.4)

    // bufferRowLength and bufferImageHeight specify the data in buffer memory as a subregion of a larger two- or three-dimensional image, and control the addressing calculations of data in buffer memory.
    // If either of these values is zero, that aspect of the buffer memory is considered to be tightly packed according to the imageExtent (18.4).
    vkBufferImageCopyRegion.bufferRowLength   = uiBufferRowStrideInTexels;
    vkBufferImageCopyRegion.bufferImageHeight = 0;

    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
    {
      // The aspectMask member of imageSubresource must only have a single bit set (18.4)

      vkBufferImageCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    else if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      // When copying to or from a depth or stencil aspect, the data in buffer memory uses a layout that is a (mostly) tightly packed representation of the depth or stencil data.
      // To copy both the depth and stencil aspects of a depth/stencil format, two entries in pRegions can be used, where one specifies the depth aspect in imageSubresource, and the other specifies the stencil aspect (18.4)
      XII_REPORT_FAILURE("Updating depth-stencil texture is not currently supported");
    }
    else
    {
      vkBufferImageCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vkBufferImageCopyRegion.imageSubresource.baseArrayLayer = uiArraySlice;
    vkBufferImageCopyRegion.imageSubresource.layerCount     = 1;
    vkBufferImageCopyRegion.imageSubresource.mipLevel       = uiMipLevel;

    // - imageOffset.x and (imageExtent.width + imageOffset.x) must both be greater than or equal to 0 and less than or equal to the image subresource width (18.4)
    // - imageOffset.y and (imageExtent.height + imageOffset.y) must both be greater than or equal to 0 and less than or equal to the image subresource height (18.4)
    vkBufferImageCopyRegion.imageOffset = vk::Offset3D{static_cast<int32_t>(region.m_vMin.x), static_cast<int32_t>(region.m_vMin.y), static_cast<int32_t>(region.m_vMin.z)};

    XII_ASSERT_DEV(region.IsValid(), "[{} .. {}) x [{} .. {}) x [{} .. {}) is not a valid region.", region.m_vMin.x, region.m_vMax.x, region.m_vMin.y, region.m_vMax.y, region.m_vMin.z, region.m_vMax.z);

    xiiVec3U32 vExtents                 = region.GetExtents();
    vkBufferImageCopyRegion.imageExtent = vk::Extent3D{vExtents.x, vExtents.y, vExtents.z};

    return vkBufferImageCopyRegion;
  }
} // namespace

void xiiGALCommandListVulkan::TransitionImageLayout(vk::Image vkImage, vk::ImageLayout vkOldLayout, vk::ImageLayout vkNewLayout, const vk::ImageSubresourceRange& vkImageSubresourceRange, vk::PipelineStageFlags vkPipelineSourceStageFlags, vk::PipelineStageFlags vkPipelineDestinationStageFlags)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");
  XII_ASSERT_DEV((vkPipelineSourceStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags) != vk::PipelineStageFlagBits::eNone, "");
  XII_ASSERT_DEV((vkPipelineDestinationStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags) != vk::PipelineStageFlagBits::eNone, "");

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
    const vk::ImageMemoryBarrier& vkImageMemoryBarrier = m_ImageBarriers[i];
    if (vkImageMemoryBarrier.image != vkImage)
      continue;

    const vk::ImageSubresourceRange& vkOtherRange = vkImageMemoryBarrier.subresourceRange;

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
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");
  XII_ASSERT_DEV((vkPipelineSourceStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags) != vk::PipelineStageFlagBits::eNone, "");
  XII_ASSERT_DEV((vkPipelineDestinationStageFlags & m_PipelineBarrier.m_vkSupportedStageFlags) != vk::PipelineStageFlagBits::eNone, "");

  m_PipelineBarrier.m_vkMemorySourceStages |= vkPipelineSourceStageFlags;
  m_PipelineBarrier.m_vkMemoryDestinationStages |= vkPipelineDestinationStageFlags;

  m_PipelineBarrier.m_vkMemorySourceAccess |= vkSourceAccessFlags;
  m_PipelineBarrier.m_vkMemoryDestinationAccess |= vkDestinationAccessFlags;
}

void xiiGALCommandListVulkan::FlushBarriers()
{
  if (m_PipelineBarrier.m_vkMemorySourceStages == vk::PipelineStageFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationStages == vk::PipelineStageFlagBits::eNone && m_ImageBarriers.IsEmpty())
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  vk::MemoryBarrier vkMemoryBarrier = {};
  vkMemoryBarrier.pNext             = nullptr;
  vkMemoryBarrier.srcAccessMask     = m_PipelineBarrier.m_vkMemorySourceAccess & m_PipelineBarrier.m_vkSupportedAccessFlags;
  vkMemoryBarrier.dstAccessMask     = m_PipelineBarrier.m_vkMemoryDestinationAccess & m_PipelineBarrier.m_vkSupportedAccessFlags;

  const bool bHasMemoryBarrier = m_PipelineBarrier.m_vkMemorySourceStages != vk::PipelineStageFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationStages == vk::PipelineStageFlagBits::eNone &&
    m_PipelineBarrier.m_vkMemorySourceAccess == vk::AccessFlagBits::eNone && m_PipelineBarrier.m_vkMemoryDestinationAccess == vk::AccessFlagBits::eNone;

  const vk::PipelineStageFlags vkSourceStages      = (m_PipelineBarrier.m_vkImageSourceStages | m_PipelineBarrier.m_vkMemorySourceStages) & m_PipelineBarrier.m_vkSupportedStageFlags;
  const vk::PipelineStageFlags vkDestinationStages = (m_PipelineBarrier.m_vkImageDestinationStages | m_PipelineBarrier.m_vkMemoryDestinationStages) & m_PipelineBarrier.m_vkSupportedStageFlags;

  XII_ASSERT_DEV(vkSourceStages != vk::PipelineStageFlagBits::eNone && vkDestinationStages != vk::PipelineStageFlagBits::eNone, "");

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

void xiiGALCommandListVulkan::CopyBufferToTexture(vk::Buffer vkSourceBuffer, xiiUInt64 uiSourceBufferOffset, xiiUInt32 uiSourceBufferRowStrideInTexels, xiiGALTextureVulkan* pDestinationTextureVulkan, const xiiBoundingBoxU32& destinationRegion, xiiUInt32 uiDestinationMipLevel, xiiUInt32 uiDestinationArraySlice, bool bVerifyOnly /*= false*/)
{
  XII_IGNORE_UNUSED(bVerifyOnly);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  TransitionOrVerifyTextureState(pDestinationTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Using texture as transfer destination (xiiGALCommandList::CopyTexture)");

  const xiiGALTextureCreationDescription& textureDescription = pDestinationTextureVulkan->GetDescription();

  vk::BufferImageCopy vkBufferImageCopy = GetBufferImageCopyInfo(uiSourceBufferOffset, uiSourceBufferRowStrideInTexels, textureDescription, destinationRegion, uiDestinationMipLevel, uiDestinationArraySlice);

  CopyBufferToImage(vkSourceBuffer, pDestinationTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, xiiMakeArrayPtr(&vkBufferImageCopy, 1U));
}

void xiiGALCommandListVulkan::CopyTextureToBuffer(xiiGALTextureVulkan* pSourceTextureVulkan, const xiiBoundingBoxU32& sourceRegion, xiiUInt32 uiSourceMipLevel, xiiUInt32 uiSourceArraySlice, vk::Buffer vkDestinationBuffer, xiiUInt64 uiDestinationBufferOffset, xiiUInt32 uiDestinationBufferRowStrideInTexels, bool bVerifyOnly /*= false*/)
{
  XII_IGNORE_UNUSED(bVerifyOnly);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  TransitionOrVerifyTextureState(pSourceTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, vk::ImageLayout::eTransferSrcOptimal, "Using texture as transfer source (xiiGALCommandList::CopyTexture)");

  const xiiGALTextureCreationDescription& textureDescription = pSourceTextureVulkan->GetDescription();

  vk::BufferImageCopy vkBufferImageCopy = GetBufferImageCopyInfo(uiDestinationBufferOffset, uiDestinationBufferRowStrideInTexels, textureDescription, sourceRegion, uiSourceMipLevel, uiSourceArraySlice);

  CopyImageToBuffer(pSourceTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferSrcOptimal, vkDestinationBuffer, xiiMakeArrayPtr(&vkBufferImageCopy, 1U));
}

void xiiGALCommandListVulkan::UpdateBufferRegion(xiiGALBufferVulkan* pBufferVulkan, vk::Buffer vkSourceBuffer, xiiUInt64 uiSourceOffset, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSizeInBytes)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV((uiDestinationOffset + uiSizeInBytes) <= pBufferVulkan->GetSize(), "Update region is out of buffer range which will result in undefined behavior.");
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  TransitionOrVerifyBufferState(pBufferVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Updating buffer (xiiGALCommandListVulkan::UpdateBufferRegion)");

  vk::BufferCopy vkBufferCopyRegion = {};
  vkBufferCopyRegion.srcOffset      = uiSourceOffset;
  vkBufferCopyRegion.dstOffset      = uiDestinationOffset;
  vkBufferCopyRegion.size           = uiSizeInBytes;

  FlushBarriers();

  m_vkCommandBuffer.copyBuffer(vkSourceBuffer, pBufferVulkan->GetVulkanBuffer(), 1U, &vkBufferCopyRegion, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyBufferToImage(vk::Buffer vkSourceBuffer, vk::Image vkDestinationImage, vk::ImageLayout vkDestinationImageLayout, xiiArrayPtr<const vk::BufferImageCopy> pRegions)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  FlushBarriers();

  m_vkCommandBuffer.copyBufferToImage(vkSourceBuffer, vkDestinationImage, vkDestinationImageLayout, pRegions.GetCount(), pRegions.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyImageToBuffer(vk::Image vkSourceImage, vk::ImageLayout vkSourceImageLayout, vk::Buffer vkDestinationBuffer, xiiArrayPtr<const vk::BufferImageCopy> pRegions)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  FlushBarriers();

  m_vkCommandBuffer.copyImageToBuffer(vkSourceImage, vkSourceImageLayout, vkDestinationBuffer, pRegions.GetCount(), pRegions.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyImage(vk::Image vkSourceImage, vk::ImageLayout vkSourceImageLayout, vk::Image vkDestinationImage, vk::ImageLayout vkDestinationImageLayout, xiiArrayPtr<const vk::ImageCopy> pRegions)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  FlushBarriers();

  m_vkCommandBuffer.copyImage(vkSourceImage, vkSourceImageLayout, vkDestinationImage, vkDestinationImageLayout, pRegions.GetCount(), pRegions.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyTextureRegion(xiiGALTextureVulkan* pSourceTextureVulkan, xiiGALTextureVulkan* pDestinationTextureVulkan, const vk::ImageCopy& copyRegion)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Buffer and Texture updates and copies are not permitted while a render pass is active.");

  TransitionOrVerifyTextureState(pSourceTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, vk::ImageLayout::eTransferSrcOptimal, "Using texture as transfer source (xiiGALCommandList::CopyTextureRegion)");
  TransitionOrVerifyTextureState(pDestinationTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Using texture as transfer destination (xiiGALCommandList::CopyTextureRegion)");

  // srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
  // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.3)
  CopyImage(pSourceTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferSrcOptimal, pDestinationTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, xiiMakeArrayPtr(&copyRegion, 1U));
}

void xiiGALCommandListVulkan::UpdateTextureRegion(const void* pSourceData, xiiUInt64 uiSourceStride, xiiUInt64 uiSourceDepthStride, xiiGALTextureVulkan* pTextureVulkan, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox)
{
  xiiSharedPtr<xiiGALDeviceVulkan>        pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*               pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  const xiiGALTextureCreationDescription& textureDescription     = pTextureVulkan->GetDescription();

  XII_ASSERT_DEV(textureDescription.m_uiSampleCount == 1U, "Only single-sample textures can be updated with vkCmdCopyBufferToImage().");

  const vk::PhysicalDeviceLimits&            deviceLimits                   = pDeviceVulkan->GetVulkanPhysicalDeviceProperties().limits;
  const xiiGALBufferToTextureCopyDescription bufferToTextureCopyDescription = xiiGALTextureUtilities::GetBufferToTextureCopyDescription(textureDescription.m_Format, destinationBox, static_cast<xiiUInt32>(deviceLimits.optimalBufferCopyRowPitchAlignment));
  const xiiUInt32                            uiUpdateRegionDepth            = bufferToTextureCopyDescription.m_Region.GetExtents().z;

  // The allocation will stay in the upload heap until the end of the frame at which point all upload pages will be discarded.
  xiiGALStagingBufferAllocationVulkan stagingBufferAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(bufferToTextureCopyDescription.m_uiMemorySize);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    XII_ASSERT_DEBUG(uiSourceStride >= bufferToTextureCopyDescription.m_uiRowSize, "Source data stride ({}) is below the image row size ({}).", uiSourceStride, bufferToTextureCopyDescription.m_uiRowSize);

    const xiiUInt64 uiPlaneSize = uiSourceStride * xiiUInt64{bufferToTextureCopyDescription.m_uiRowCount};
    XII_ASSERT_DEBUG(uiUpdateRegionDepth == 1 || uiSourceDepthStride >= uiPlaneSize, "Source data depth stride ({}) is below the image plane size ({}).", uiSourceDepthStride, uiPlaneSize);
  }
#endif

  void* pMappedMemory = nullptr;
  VK_SUCCEED_OR_RETURN(pVulkanMemoryAllocator->MapMemory(stagingBufferAllocation.m_VulkanAllocation, &pMappedMemory));
  VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, bufferToTextureCopyDescription.m_uiMemorySize));

  pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);

  for (xiiUInt32 uiDepthSlice = 0; uiDepthSlice < uiUpdateRegionDepth; ++uiDepthSlice)
  {
    for (xiiUInt32 uiRow = 0; uiRow < bufferToTextureCopyDescription.m_uiRowCount; ++uiRow)
    {
      xiiMemoryUtils::RawByteCopy(xiiMemoryUtils::AddByteOffset(reinterpret_cast<xiiUInt8*>(pMappedMemory), uiRow * bufferToTextureCopyDescription.m_uiRowStride + uiDepthSlice * bufferToTextureCopyDescription.m_uiDepthStride), xiiMemoryUtils::AddByteOffset(reinterpret_cast<const xiiUInt8*>(pSourceData), uiRow * uiSourceStride + uiDepthSlice * uiSourceDepthStride), bufferToTextureCopyDescription.m_uiRowSize);
    }
  }

  VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, bufferToTextureCopyDescription.m_uiMemorySize));
  pVulkanMemoryAllocator->UnmapMemory(stagingBufferAllocation.m_VulkanAllocation);

  CopyBufferToTexture(stagingBufferAllocation.m_vkBuffer, stagingBufferAllocation.m_uiOffset, bufferToTextureCopyDescription.m_uiRowStrideInTexels, pTextureVulkan, bufferToTextureCopyDescription.m_Region, uiMipLevel, uiSlice);
}

void xiiGALCommandListVulkan::AddWaitSemaphore(vk::Semaphore vkSemaphore, vk::PipelineStageFlags pipelineFlags, xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(vkSemaphore != VK_NULL_HANDLE, "");

  m_vkWaitSemaphores.PushBack(vkSemaphore);
  m_vkWaitDestinationStageFlags.PushBack(pipelineFlags);
  m_vkWaitSemaphoreValues.PushBack(uiValue); // Ignored for binary semaphore.
}

void xiiGALCommandListVulkan::AddSignalSemaphore(vk::Semaphore vkSemaphore, xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(vkSemaphore != VK_NULL_HANDLE, "");

  m_vkSignalSemaphores.PushBack(vkSemaphore);
  m_vkSignalSemaphoreValues.PushBack(uiValue); // Ignored for binary semaphore.
}

xiiGALCommandListVulkan::xiiGALCommandListVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALCommandListVulkan::~xiiGALCommandListVulkan()
{
  Reset();
}

xiiResult xiiGALCommandListVulkan::InitPlatform()
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALCommandBufferPoolVulkan*   pCommandBufferPool = pDeviceVulkan->GetCommandBufferPool(m_Description.m_QueueFlags);

  {
    m_CommandListData                            = {};
    m_CommandListData.m_pDynamicBufferPoolVulkan = XII_NEW(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator(), xiiGALDynamicBufferPoolVulkan, static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow()), 16U, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
    m_CommandListData.m_pUploadStagingBufferPool = XII_NEW(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator(), xiiGALStagingBufferPoolVulkan, static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow()), 16U, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
    m_CommandListData.m_pDescriptorSetPoolVulkan = XII_NEW(static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow())->GetAllocator(), xiiGALDescriptorSetPoolVulkan, static_cast<xiiGALDeviceVulkan*>(m_pDevice.Borrow()), 32U, 16U);

    {
      xiiGALBufferCreationDescription nullVertexBufferDescription;
      nullVertexBufferDescription.m_BindFlags = xiiGALBindFlags::VertexBuffer;
      nullVertexBufferDescription.m_Usage     = xiiGALResourceUsage::Mutable;
      nullVertexBufferDescription.m_uiSize    = 32U;

      xiiSharedPtr<xiiGALBuffer> pNullVertexBuffer = m_pDevice->CreateBuffer(nullVertexBufferDescription);
      pNullVertexBuffer->SetDebugName("Null Vertex Buffer");

      m_CommandListData.m_pNullVertexBuffer = pNullVertexBuffer.Downcast<xiiGALBufferVulkan>();
    }
  }

  if (m_Description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    m_CommandBufferAllocation = pCommandBufferPool->AllocateSecondaryCommandBuffer();
  }
  else
  {
    m_CommandBufferAllocation = pCommandBufferPool->AllocatePrimaryCommandBuffer();
  }

  m_vkCommandBuffer = m_CommandBufferAllocation.Get();
  XII_ASSERT_DEV(m_vkCommandBuffer != VK_NULL_HANDLE, "Failed to allocate a Vulkan command buffer.");

  m_PipelineBarrier.m_vkSupportedStageFlags  = pDeviceVulkan->GetSupportedStagesFlags(m_Description.m_QueueFlags);
  m_PipelineBarrier.m_vkSupportedAccessFlags = pDeviceVulkan->GetSupportedAccessFlags(m_Description.m_QueueFlags);

  vk::CommandBufferUsageFlagBits vkCommandBufferUsageFlags = m_Description.m_Flags.IsSet(xiiGALCommandListFlags::MultiSubmit) ? vk::CommandBufferUsageFlagBits::eSimultaneousUse : vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  vk::CommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.pNext                      = nullptr;
  vkCommandBufferBeginInfo.flags                      = vkCommandBufferUsageFlags;
  vkCommandBufferBeginInfo.pInheritanceInfo           = nullptr; // Ignored for a primary command buffer.

  vk::CommandBufferInheritanceInfo vkCommandBufferInheritanceInfo = {};
  vkCommandBufferInheritanceInfo.pNext                            = nullptr;
  vkCommandBufferInheritanceInfo.subpass                          = m_Description.m_uiSubPassIndex;

  if (m_Description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    vkCommandBufferBeginInfo.flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;

    if (xiiGALRenderPassVulkan* pRenderPassVulkan = xiiDynamicCast<xiiGALRenderPassVulkan*>(m_Description.m_pRenderPass))
    {
      vkCommandBufferInheritanceInfo.renderPass = pRenderPassVulkan->GetVulkanRenderPass();
    }
    if (xiiGALFramebufferVulkan* pFramebufferVulkan = xiiDynamicCast<xiiGALFramebufferVulkan*>(m_Description.m_pFramebuffer))
    {
      vkCommandBufferInheritanceInfo.framebuffer = pFramebufferVulkan->GetVulkanFramebuffer();
    }
    vkCommandBufferBeginInfo.pInheritanceInfo = &vkCommandBufferInheritanceInfo;
  }

  VK_ASSERT_DEV(m_vkCommandBuffer.begin(&vkCommandBufferBeginInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  xiiGALQueryPoolVulkan* pQueryPoolVulkan = pDeviceVulkan->GetCommandQueueQueryPool(m_Description.m_QueueFlags);
  pQueryPoolVulkan->ResetStaleQueries(m_vkCommandBuffer);

  m_RecordingState = RecordingState::Recording;
}

void xiiGALCommandListVulkan::EndPlatform()
{
  XII_ASSERT_DEV(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  FlushBarriers();

  VK_ASSERT_DEV(m_vkCommandBuffer.end(pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_RecordingState = RecordingState::Ended;
}

void xiiGALCommandListVulkan::ResetPlatform()
{
  // For one-time submit command buffers, they are invalidated once submitted to a queue.
  if (m_vkCommandBuffer != VK_NULL_HANDLE)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    xiiGALCommandBufferPoolVulkan*   pCommandBufferPoolVulkan = pDeviceVulkan->GetCommandBufferPool(m_Description.m_QueueFlags);

    pCommandBufferPoolVulkan->RecycleAfterSubmit(std::move(m_CommandBufferAllocation), std::move(m_CommandListData), pDeviceVulkan->GetCommandQueue(m_Description.m_QueueFlags)->GetNextFenceValue());
  }

  m_CommandBufferAllocation = {};
  m_vkCommandBuffer         = VK_NULL_HANDLE;
  m_CommandListData         = {};

  m_RecordingState = RecordingState::Reset;
}

void xiiGALCommandListVulkan::SubmitPlatform(xiiGALCommandList* pSecondaryCommandList)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALCommandListVulkan*         pCommandListVulkan = xiiDynamicCast<xiiGALCommandListVulkan*>(pSecondaryCommandList);

  vk::CommandBuffer vkCommandBuffer = pCommandListVulkan->GetVulkanCommandBuffer();

  m_vkCommandBuffer.executeCommands(1U, &vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  XII_IGNORE_UNUSED(pPipelineState);

  m_CommandListData.m_bPipelineStateModified = true;
}

void xiiGALCommandListVulkan::PushConstantsPlatform(xiiUInt32 uiOffset, xiiArrayPtr<const xiiUInt8> pData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  const auto& pushRanges = m_pPipelineResourceSignature->GetDescription().m_PushConstantRanges;

  bool                 bFound       = false;
  vk::ShaderStageFlags vkStageFlags = {};

  for (const xiiGALPushConstantRange& range : pushRanges)
  {
    const xiiUInt32 uiRangeStart = range.m_uiOffset;
    const xiiUInt32 uiRangeEnd   = range.m_uiOffset + range.m_uiSize;

    if (uiOffset >= uiRangeStart && (uiOffset + pData.GetCount()) <= uiRangeEnd)
    {
      vkStageFlags = xiiVulkanTypeConversions::GetShaderStageFlags(range.m_ShaderStages);
      bFound       = true;
      break;
    }
  }

  if (!bFound)
  {
    xiiLog::Error("PushConstants: No matching push constant range found for offset {} with size {}.", uiOffset, pData.GetCount());
    return;
  }

  vk::PipelineLayout vkPipelineLayout = VK_NULL_HANDLE;

  const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();

  if (pipelineDescription.IsAnyGraphicsPipeline())
  {
    xiiGALGraphicsPipelineStateVulkan* pGraphics = xiiDynamicCast<xiiGALGraphicsPipelineStateVulkan*>(m_pPipelineState);
    vkPipelineLayout                             = pGraphics->GetVulkanPipelineLayout();
  }
  else if (pipelineDescription.IsComputePipeline())
  {
    xiiGALComputePipelineStateVulkan* pCompute = xiiDynamicCast<xiiGALComputePipelineStateVulkan*>(m_pPipelineState);
    vkPipelineLayout                           = pCompute->GetVulkanPipelineLayout();
  }
  else if (pipelineDescription.IsRayTracingPipeline())
  {
    xiiGALRayTracingPipelineStateVulkan* pRT = xiiDynamicCast<xiiGALRayTracingPipelineStateVulkan*>(m_pPipelineState);
    vkPipelineLayout                         = pRT->GetVulkanPipelineLayout();
  }

  if (vkPipelineLayout == VK_NULL_HANDLE)
  {
    xiiLog::Error("PushConstants: Pipeline layout is null.");
    return;
  }

  m_vkCommandBuffer.pushConstants(vkPipelineLayout, vkStageFlags, uiOffset, pData.GetCount(), pData.GetPtr(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  XII_ASSERT_DEBUG(m_vkCommandBuffer != VK_NULL_HANDLE, "Invalid command buffer.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  m_vkCommandBuffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, uiStencilRef, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  XII_ASSERT_DEBUG(m_vkCommandBuffer != VK_NULL_HANDLE, "Invalid command buffer.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  m_vkCommandBuffer.setBlendConstants(blendFactor.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
  XII_ASSERT_DEBUG(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");
  XII_ASSERT_DEBUG(m_vkCommandBuffer != VK_NULL_HANDLE, "Invalid command buffer.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  xiiTemporaryHybridArray<vk::Viewport, 2U> vkViewPorts;
  vkViewPorts.SetCountUninitialized(pViewports.GetCount());

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
    //         0 _   (0,0)_______________________(1,0)                  Tex Height _     (0,1)_______________________(1,1)
    //         1 _       |                       |      |             VP Top + Height _ _ _ _|   __________          |      A
    //         2 _       |                       |      |                          .         |  |   .--> +x|         |      |
    //           .       |                       |      |                          .         |  |   |      |         |      |
    //           .       |                       |      | V Coord                  .         |  |   V +y   |         |      | V Coord
    //     VP Top _ _ _ _|   __________          |      |                    VP Top   _ _ _ _|  |__________|         |      |
    //           .       |  |    A +y  |         |      |                          .         |                       |      |
    //           .       |  |    |     |         |      |                          .         |                       |      |
    //           .       |  |    '-->+x|         |      |                        2 _         |                       |      |
    //           .       |  |__________|         |      |                        1 _         |                       |      |
    //Tex Height _       |_______________________|      V                        0 _         |_______________________|      |
    //               (0,1)                       (1,1)                                 (0,0)                       (1,0)
    //
    //

    vkViewPorts[uiViewPortIndex].y      = vkViewPorts[uiViewPortIndex].y + vkViewPorts[uiViewPortIndex].height;
    vkViewPorts[uiViewPortIndex].height = -vkViewPorts[uiViewPortIndex].height;
  }

  m_vkCommandBuffer.setViewport(0, vkViewPorts.GetCount(), vkViewPorts.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
  XII_ASSERT_DEBUG(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");
  XII_ASSERT_DEBUG(m_vkCommandBuffer != VK_NULL_HANDLE, "Invalid command buffer.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  xiiTemporaryHybridArray<vk::Rect2D, 2U> vkScissorRects;
  vkScissorRects.SetCountUninitialized(pRects.GetCount());

  for (xiiUInt32 uiScissorRectIndex = 0; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    vkScissorRects[uiScissorRectIndex].offset = vk::Offset2D{static_cast<xiiInt32>(pRects[uiScissorRectIndex].x), static_cast<xiiInt32>(pRects[uiScissorRectIndex].y)};
    vkScissorRects[uiScissorRectIndex].extent = vk::Extent2D{pRects[uiScissorRectIndex].width, pRects[uiScissorRectIndex].height};
  }

  m_vkCommandBuffer.setScissor(0, vkScissorRects.GetCount(), vkScissorRects.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
  XII_IGNORE_UNUSED(uiByteOffset);

  if (pIndexBuffer)
  {
    xiiGALBufferVulkan* pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(pIndexBuffer);

    TransitionOrVerifyBufferState(pBufferVulkan, transitionMode, xiiGALResourceStateFlags::IndexBuffer, vk::AccessFlagBits::eIndexRead, "Binding buffer as index buffer (xiiGALCommandListVulkan::SetIndexBuffer)");
  }

  m_CommandListFlags.Add(CommandListFlags::CommittedIndexBufferModified);
}

void xiiGALCommandListVulkan::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<VertexStreamDescription> pVertexStreams, xiiBitflags<xiiGALSetVertexBufferFlags> flags, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
  XII_IGNORE_UNUSED(uiStartSlot);
  XII_IGNORE_UNUSED(flags);

  for (xiiUInt32 uiSlot = 0U; uiSlot < pVertexStreams.GetCount(); ++uiSlot)
  {
    VertexStreamDescription& vertexStream = pVertexStreams[uiSlot];

    if (xiiGALBufferVulkan* pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(vertexStream.m_pBuffer))
    {
      TransitionOrVerifyBufferState(pBufferVulkan, transitionMode, xiiGALResourceStateFlags::VertexBuffer, vk::AccessFlagBits::eVertexAttributeRead, "Setting vertex buffers (xiiGALCommandListVulkan::SetVertexBuffers)");
    }
  }

  m_CommandListFlags.Add(CommandListFlags::CommittedVertexBuffersModified);
}

void xiiGALCommandListVulkan::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
  xiiGALBufferVulkan* pConstantBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(pConstantBuffer);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundConstantBuffers.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundConstantBuffers[bindingInformation.m_uiBindSlot] = pConstantBufferVulkan != nullptr ? pConstantBufferVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = xiiDynamicCast<xiiGALBufferViewVulkan*>(pBufferView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundBufferResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundBufferResourceViews[bindingInformation.m_uiBindSlot] = pBufferViewVulkan != nullptr ? pBufferViewVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = xiiDynamicCast<xiiGALTextureViewVulkan*>(pTextureView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundTextureResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundTextureResourceViews[bindingInformation.m_uiBindSlot] = pTextureViewVulkan != nullptr ? pTextureViewVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = xiiDynamicCast<xiiGALBufferViewVulkan*>(pBufferView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundUnorderedAccessBufferResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundUnorderedAccessBufferResourceViews[bindingInformation.m_uiBindSlot] = pBufferViewVulkan != nullptr ? pBufferViewVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = xiiDynamicCast<xiiGALTextureViewVulkan*>(pTextureView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundUnorderedAccessTextureResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundUnorderedAccessTextureResourceViews[bindingInformation.m_uiBindSlot] = pTextureViewVulkan != nullptr ? pTextureViewVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
  xiiGALSamplerVulkan* pSamplerVulkan = xiiDynamicCast<xiiGALSamplerVulkan*>(pSampler);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundSamplerStates.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundSamplerStates[bindingInformation.m_uiBindSlot] = pSamplerVulkan != nullptr ? pSamplerVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListVulkan::SetAccelerationStructurePlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = xiiDynamicCast<xiiGALTopLevelASVulkan*>(pTopLevelAS);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1);

  auto& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundAccelerationStructures.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  bindSetResources.m_pBoundAccelerationStructures[bindingInformation.m_uiBindSlot] = pTopLevelASVulkan != nullptr ? pTopLevelASVulkan : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

xiiResult xiiGALCommandListVulkan::CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  if (m_CommandListData.m_bPipelineStateModified)
  {
    if (m_pPipelineState != nullptr)
    {
      const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();

      if (pipelineDescription.IsAnyGraphicsPipeline())
      {
        xiiGALGraphicsPipelineStateVulkan* pGraphicsPipelineStateVulkan = xiiDynamicCast<xiiGALGraphicsPipelineStateVulkan*>(m_pPipelineState);

        m_CommandListState.m_vkGraphicsPipeline = pGraphicsPipelineStateVulkan->GetVulkanPipeline();

        m_vkCommandBuffer.bindPipeline(pGraphicsPipelineStateVulkan->GetVulkanPipelineBindPoint(), pGraphicsPipelineStateVulkan->GetVulkanPipeline(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      else if (pipelineDescription.IsComputePipeline())
      {
        xiiGALComputePipelineStateVulkan* pComputePipelineStateVulkan = xiiDynamicCast<xiiGALComputePipelineStateVulkan*>(m_pPipelineState);

        m_CommandListState.m_vkComputePipeline = pComputePipelineStateVulkan->GetVulkanPipeline();

        m_vkCommandBuffer.bindPipeline(pComputePipelineStateVulkan->GetVulkanPipelineBindPoint(), pComputePipelineStateVulkan->GetVulkanPipeline(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      else if (pipelineDescription.IsRayTracingPipeline())
      {
        xiiGALRayTracingPipelineStateVulkan* pRayTracingPipelineStateVulkan = xiiDynamicCast<xiiGALRayTracingPipelineStateVulkan*>(m_pPipelineState);

        m_CommandListState.m_vkRayTracingPipeline = pRayTracingPipelineStateVulkan->GetVulkanPipeline();

        m_vkCommandBuffer.bindPipeline(pRayTracingPipelineStateVulkan->GetVulkanPipelineBindPoint(), pRayTracingPipelineStateVulkan->GetVulkanPipeline(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
    }

    m_CommandListData.m_bPipelineStateModified = false;
    m_CommandListData.m_bDescriptorsModified   = true; // Changes to the descriptor layout always require the descriptor set to be re-created.
  }

  if (m_CommandListData.m_bDescriptorsModified)
  {
    m_CommandListData.m_DynamicUniformBuffers.Clear();
    m_CommandListData.m_DynamicUniformBufferOffsets.Clear();

    if (m_pPipelineResourceSignature != nullptr)
    {
      xiiGALPipelineResourceSignatureVulkan* pResourceSignatureVulkan = xiiDynamicCast<xiiGALPipelineResourceSignatureVulkan*>(m_pPipelineResourceSignature);

      m_CommandListData.m_DescriptorSets.SetCountUninitialized(pResourceSignatureVulkan->GetVulkanDescriptorSetLayoutCount());

      for (xiiUInt32 uiSet = 0; uiSet < m_CommandListData.m_DescriptorSets.GetCount(); ++uiSet)
      {
        m_CommandListData.m_DescriptorSets[uiSet] = m_CommandListData.m_pDescriptorSetPoolVulkan->RequestDescriptorSet(pResourceSignatureVulkan->GetVulkanDescriptorSetLayout(uiSet));

        auto& resources = m_CommandListData.m_ResourceSets[uiSet];

        const auto pPipelineResourceLayout = pResourceSignatureVulkan->GetPipelineResourceSetLayout(uiSet);
        for (xiiUInt32 i = 0; i < pPipelineResourceLayout.GetCount(); ++i)
        {
          const xiiGALPipelineResourceDescriptionVulkan& resourceLayout = pPipelineResourceLayout[i];

          vk::WriteDescriptorSet vkWriteDescriptorSet = {};
          vkWriteDescriptorSet.pNext                  = nullptr;
          vkWriteDescriptorSet.dstSet                 = m_CommandListData.m_DescriptorSets[uiSet];
          vkWriteDescriptorSet.dstBinding             = resourceLayout.m_uiBindingIndex;
          vkWriteDescriptorSet.dstArrayElement        = 0U; // Resource arrays are written from element 0 using descriptorCount.
          vkWriteDescriptorSet.descriptorCount        = resourceLayout.m_uiArraySize;
          vkWriteDescriptorSet.descriptorType         = xiiVulkanTypeConversions::GetDescriptorType(resourceLayout.m_DescriptorType); // descriptorType must be the same type as that specified in VkDescriptorSetLayoutBinding for dstSet at dstBinding. The type of the descriptor also controls which array the descriptors are taken from. (13.2.4)
          vkWriteDescriptorSet.pImageInfo             = nullptr;
          vkWriteDescriptorSet.pBufferInfo            = nullptr;
          vkWriteDescriptorSet.pTexelBufferView       = nullptr;

          vk::DescriptorImageInfo                        vkDescriptorImageInfo;
          vk::DescriptorBufferInfo                       vkDescriptorBufferInfo;
          vk::BufferView                                 vkDescriptorBufferView;
          vk::WriteDescriptorSetAccelerationStructureKHR vkDescriptorAccelStructInfo;

          switch (resourceLayout.m_DescriptorType)
          {
            case xiiGALDescriporTypeVulkan::UniformBuffer:
            case xiiGALDescriporTypeVulkan::UniformBufferDynamic:
            {
              if (xiiGALBufferVulkan* pBufferVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundConstantBuffers.GetCount() ? resources.m_pBoundConstantBuffers[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                vkDescriptorBufferInfo        = vk::DescriptorBufferInfo{};
                vkDescriptorBufferInfo.buffer = pBufferVulkan->GetVulkanBuffer();
                vkDescriptorBufferInfo.offset = 0;
                vkDescriptorBufferInfo.range  = pBufferVulkan->GetSize();

                vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::ConstantBuffer, true);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No constant buffer bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }

              if (resourceLayout.m_DescriptorType == xiiGALDescriporTypeVulkan::UniformBufferDynamic)
              {
                // Move offset out and into the separate offset array.
                auto& bufferInfo = m_CommandListData.m_DynamicUniformBuffers.ExpandAndGetRef();
                bufferInfo       = *vkWriteDescriptorSet.pBufferInfo;

                m_CommandListData.m_DynamicUniformBufferOffsets.PushBack((xiiUInt32)bufferInfo.offset);

                bufferInfo.offset                = 0U;
                vkWriteDescriptorSet.pBufferInfo = &bufferInfo;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::UniformTexelBuffer:
            case xiiGALDescriporTypeVulkan::StorageTexelBufferReadOnly:
            {
              if (const xiiGALBufferViewVulkan* pBufferViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundBufferResourceViews.GetCount() ? resources.m_pBoundBufferResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALBufferVulkan* pBufferVulkan = pBufferViewVulkan->GetBuffer().Downcast<xiiGALBufferVulkan>();

                vkDescriptorBufferView                = pBufferViewVulkan->GetVulkanBufferView();
                vkWriteDescriptorSet.pTexelBufferView = &vkDescriptorBufferView;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::ShaderResource, true);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No buffer resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::StorageTexelBuffer:
            {
              if (const xiiGALBufferViewVulkan* pBufferViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundUnorderedAccessBufferResourceViews.GetCount() ? resources.m_pBoundUnorderedAccessBufferResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALBufferVulkan* pBufferVulkan = pBufferViewVulkan->GetBuffer().Downcast<xiiGALBufferVulkan>();

                vkDescriptorBufferView                = pBufferViewVulkan->GetVulkanBufferView();
                vkWriteDescriptorSet.pTexelBufferView = &vkDescriptorBufferView;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::UnorderedAccess, true);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No unordered access buffer resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::StorageBufferReadOnly:
            case xiiGALDescriporTypeVulkan::StorageBufferDynamicReadOnly:
            {
              if (const xiiGALBufferViewVulkan* pBufferViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundBufferResourceViews.GetCount() ? resources.m_pBoundBufferResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALBufferVulkan* pBufferVulkan = pBufferViewVulkan->GetBuffer().Downcast<xiiGALBufferVulkan>();

                vkDescriptorBufferInfo        = vk::DescriptorBufferInfo{};
                vkDescriptorBufferInfo.buffer = pBufferVulkan->GetVulkanBuffer();
                vkDescriptorBufferInfo.offset = 0;
                vkDescriptorBufferInfo.range  = pBufferVulkan->GetSize();

                vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::ShaderResource, true);

                  FlushBarriers();
                }

                if (resourceLayout.m_DescriptorType == xiiGALDescriporTypeVulkan::StorageBufferDynamicReadOnly)
                {
                  // Move offset out and into the separate offset array.
                  auto& bufferInfo = m_CommandListData.m_DynamicUniformBuffers.ExpandAndGetRef();
                  bufferInfo       = *vkWriteDescriptorSet.pBufferInfo;

                  m_CommandListData.m_DynamicUniformBufferOffsets.PushBack((xiiUInt32)bufferInfo.offset);

                  bufferInfo.offset                = 0U;
                  vkWriteDescriptorSet.pBufferInfo = &bufferInfo;
                }
              }
              else
              {
                xiiLog::Error("No buffer resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::StorageBuffer:
            case xiiGALDescriporTypeVulkan::StorageBufferDynamic:
            {
              if (const xiiGALBufferViewVulkan* pBufferViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundUnorderedAccessBufferResourceViews.GetCount() ? resources.m_pBoundUnorderedAccessBufferResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALBufferVulkan* pBufferVulkan = pBufferViewVulkan->GetBuffer().Downcast<xiiGALBufferVulkan>();

                vkDescriptorBufferInfo        = vk::DescriptorBufferInfo{};
                vkDescriptorBufferInfo.buffer = pBufferVulkan->GetVulkanBuffer();
                vkDescriptorBufferInfo.offset = 0;
                vkDescriptorBufferInfo.range  = pBufferVulkan->GetSize();

                vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::UnorderedAccess, true);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No unordered access buffer resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }

              if (resourceLayout.m_DescriptorType == xiiGALDescriporTypeVulkan::StorageBufferDynamic)
              {
                // Move offset out and into the separate offset array.
                auto& bufferInfo = m_CommandListData.m_DynamicUniformBuffers.ExpandAndGetRef();
                bufferInfo       = *vkWriteDescriptorSet.pBufferInfo;

                m_CommandListData.m_DynamicUniformBufferOffsets.PushBack((xiiUInt32)bufferInfo.offset);

                bufferInfo.offset                = 0U;
                vkWriteDescriptorSet.pBufferInfo = &bufferInfo;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::CombinedImageSampler:
            {
              if (const xiiGALTextureViewVulkan* pTextureViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundTextureResourceViews.GetCount() ? resources.m_pBoundTextureResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALTextureVulkan* pTextureVulkan = pTextureViewVulkan->GetTexture().Downcast<xiiGALTextureVulkan>();

                vkDescriptorImageInfo           = vk::DescriptorImageInfo{};
                vkDescriptorImageInfo.imageView = pTextureViewVulkan->GetVulkanImageView();

                // The image subresources for a storage image must be in the VK_IMAGE_LAYOUT_GENERAL layout in order to access its data in a shader (13.1.1)
                // The image subresources for a sampled image or a combined image sampler must be in the VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, or VK_IMAGE_LAYOUT_GENERAL layout in order to access its data in a shader (13.1.3, 13.1.4).
                if (pTextureVulkan->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
                {
                  // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL must only be used as a read - only depth / stencil attachment in a VkFramebuffer and/or as a read - only image in a shader (which can be read as a sampled image, combined
                  // image / sampler and /or input attachment). This layout is valid only for image subresources of images created with the VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT usage bit enabled. (11.4)
                  vkDescriptorImageInfo.imageLayout = xiiVulkanTypeConversions::GetImageLayout(xiiGALResourceStateFlags::DepthRead);

                  XII_ASSERT_DEV(vkDescriptorImageInfo.imageLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal, "");

                  if (mode == xiiGALStateTransitionMode::Transition)
                  {
                    TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::DepthRead, xiiGALStateTransitionFlags::UpdateState);

                    FlushBarriers();
                  }
                }
                else
                {
                  vkDescriptorImageInfo.imageLayout = xiiVulkanTypeConversions::GetImageLayout(xiiGALResourceStateFlags::ShaderResource);

                  XII_ASSERT_DEV(vkDescriptorImageInfo.imageLayout == vk::ImageLayout::eShaderReadOnlyOptimal, "");

                  if (mode == xiiGALStateTransitionMode::Transition)
                  {
                    TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::ShaderResource, xiiGALStateTransitionFlags::UpdateState);

                    FlushBarriers();
                  }
                }

                if (!resourceLayout.m_bHasImmutableSampler)
                {
                  if (const xiiGALSamplerVulkan* pSamplerVulkan = (resourceLayout.m_uiSamplerIndex < resources.m_pBoundSamplerStates.GetCount() ? resources.m_pBoundSamplerStates[resourceLayout.m_uiSamplerIndex] : nullptr))
                  {
                    vkDescriptorImageInfo.sampler = pSamplerVulkan->GetVulkanSampler();
                  }
                  else
                  {
                    xiiLog::Error("No combined image sampler bound at '{}' in slot {}.", resourceLayout.m_sName.GetView(), resourceLayout.m_uiSamplerIndex);
                    return XII_FAILURE;
                  }
                }

                vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
              }
              else
              {
                xiiLog::Error("No texture resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::SeparateImage:
            {
              if (const xiiGALTextureViewVulkan* pTextureViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundTextureResourceViews.GetCount() ? resources.m_pBoundTextureResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALTextureVulkan* pTextureVulkan = pTextureViewVulkan->GetTexture().Downcast<xiiGALTextureVulkan>();

                vkDescriptorImageInfo           = vk::DescriptorImageInfo{};
                vkDescriptorImageInfo.imageView = pTextureViewVulkan->GetVulkanImageView();

                // The image subresources for a storage image must be in the VK_IMAGE_LAYOUT_GENERAL layout in order to access its data in a shader (13.1.1)
                // The image subresources for a sampled image or a combined image sampler must be in the VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, or VK_IMAGE_LAYOUT_GENERAL layout in order to access its data in a shader (13.1.3, 13.1.4).
                if (pTextureVulkan->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
                {
                  // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL must only be used as a read - only depth / stencil attachment in a VkFramebuffer and/or as a read - only image in a shader (which can be read as a sampled image, combined
                  // image / sampler and /or input attachment). This layout is valid only for image subresources of images created with the VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT usage bit enabled. (11.4)
                  vkDescriptorImageInfo.imageLayout = xiiVulkanTypeConversions::GetImageLayout(xiiGALResourceStateFlags::DepthRead);

                  XII_ASSERT_DEV(vkDescriptorImageInfo.imageLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal, "");

                  if (mode == xiiGALStateTransitionMode::Transition)
                  {
                    TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::DepthRead, xiiGALStateTransitionFlags::UpdateState);

                    FlushBarriers();
                  }
                }
                else
                {
                  vkDescriptorImageInfo.imageLayout = xiiVulkanTypeConversions::GetImageLayout(xiiGALResourceStateFlags::ShaderResource);

                  XII_ASSERT_DEV(vkDescriptorImageInfo.imageLayout == vk::ImageLayout::eShaderReadOnlyOptimal, "");

                  if (mode == xiiGALStateTransitionMode::Transition)
                  {
                    TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::ShaderResource, xiiGALStateTransitionFlags::UpdateState);

                    FlushBarriers();
                  }
                }

                vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
              }
              else
              {
                xiiLog::Error("No texture resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::StorageImage:
            {
              if (const xiiGALTextureViewVulkan* pTextureViewVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundUnorderedAccessTextureResourceViews.GetCount() ? resources.m_pBoundUnorderedAccessTextureResourceViews[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                xiiGALTextureVulkan* pTextureVulkan = pTextureViewVulkan->GetTexture().Downcast<xiiGALTextureVulkan>();

                vkDescriptorImageInfo             = vk::DescriptorImageInfo{};
                vkDescriptorImageInfo.imageView   = pTextureViewVulkan->GetVulkanImageView();
                vkDescriptorImageInfo.imageLayout = xiiVulkanTypeConversions::GetImageLayout(xiiGALResourceStateFlags::UnorderedAccess);

                XII_ASSERT_DEV(vkDescriptorImageInfo.imageLayout == vk::ImageLayout::eGeneral, "");

                vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, xiiGALResourceStateFlags::UnorderedAccess, xiiGALStateTransitionFlags::UpdateState);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No unordered access texture resource view bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::Sampler:
            {
              if (const xiiGALSamplerVulkan* pSamplerVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundSamplerStates.GetCount() ? resources.m_pBoundSamplerStates[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                vkDescriptorImageInfo         = vk::DescriptorImageInfo{};
                vkDescriptorImageInfo.sampler = pSamplerVulkan->GetVulkanSampler();

                vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
              }
              else
              {
                xiiLog::Error("No sampler bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;
            case xiiGALDescriporTypeVulkan::AccelerationStructure:
            {
              if (xiiGALTopLevelASVulkan* pTopLevelASVulkan = (resourceLayout.m_uiBindingIndex < resources.m_pBoundAccelerationStructures.GetCount() ? resources.m_pBoundAccelerationStructures[resourceLayout.m_uiBindingIndex] : nullptr))
              {
                vk::AccelerationStructureKHR vkAccelerationStructure = pTopLevelASVulkan->GetVulkanAccelerationStructure();
                if (vkAccelerationStructure == VK_NULL_HANDLE)
                {
                  xiiLog::Error("Invalid acceleration structure bound at '{}'.", resourceLayout.m_sName.GetView());
                  return XII_FAILURE;
                }

                vkDescriptorAccelStructInfo                            = vk::WriteDescriptorSetAccelerationStructureKHR{};
                vkDescriptorAccelStructInfo.pNext                      = nullptr;
                vkDescriptorAccelStructInfo.accelerationStructureCount = 1U;
                vkDescriptorAccelStructInfo.pAccelerationStructures    = &vkAccelerationStructure;

                vkWriteDescriptorSet.pNext = &vkDescriptorAccelStructInfo;

                if (mode == xiiGALStateTransitionMode::Transition)
                {
                  xiiBitflags<xiiGALResourceStateFlags>       oldState = pTopLevelASVulkan->GetResourceState();
                  const xiiBitflags<xiiGALResourceStateFlags> newState = xiiGALResourceStateFlags::RayTracing;

                  if (oldState == xiiGALResourceStateFlags::Unknown)
                  {
                    oldState = xiiGALResourceStateFlags::BuildASWrite;
                  }

                  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(newState), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(newState));

                  pTopLevelASVulkan->SetResourceState(newState);

                  FlushBarriers();
                }
              }
              else
              {
                xiiLog::Error("No acceleration structure bound at '{}'.", resourceLayout.m_sName.GetView());
                return XII_FAILURE;
              }
            }
            break;

              XII_DEFAULT_CASE_NOT_IMPLEMENTED;
          }

          vkLogicalDevice.updateDescriptorSets(1U, &vkWriteDescriptorSet, 0, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
      }

      if (!m_CommandListData.m_DescriptorSets.IsEmpty())
      {
        const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();

        if (pipelineDescription.IsAnyGraphicsPipeline())
        {
          xiiGALGraphicsPipelineStateVulkan* pGraphicsPipelineStateVulkan = xiiDynamicCast<xiiGALGraphicsPipelineStateVulkan*>(m_pPipelineState);

          m_vkCommandBuffer.bindDescriptorSets(pGraphicsPipelineStateVulkan->GetVulkanPipelineBindPoint(), pGraphicsPipelineStateVulkan->GetVulkanPipelineLayout(), 0, m_CommandListData.m_DescriptorSets.GetCount(), m_CommandListData.m_DescriptorSets.GetData(), m_CommandListData.m_DynamicUniformBufferOffsets.GetCount(), m_CommandListData.m_DynamicUniformBufferOffsets.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
        else if (pipelineDescription.IsComputePipeline())
        {
          xiiGALComputePipelineStateVulkan* pComputePipelineStateVulkan = xiiDynamicCast<xiiGALComputePipelineStateVulkan*>(m_pPipelineState);

          m_vkCommandBuffer.bindDescriptorSets(pComputePipelineStateVulkan->GetVulkanPipelineBindPoint(), pComputePipelineStateVulkan->GetVulkanPipelineLayout(), 0, m_CommandListData.m_DescriptorSets.GetCount(), m_CommandListData.m_DescriptorSets.GetData(), m_CommandListData.m_DynamicUniformBufferOffsets.GetCount(), m_CommandListData.m_DynamicUniformBufferOffsets.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
        else if (pipelineDescription.IsRayTracingPipeline())
        {
          xiiGALRayTracingPipelineStateVulkan* pRayTracingPipelineStateVulkan = xiiDynamicCast<xiiGALRayTracingPipelineStateVulkan*>(m_pPipelineState);

          m_vkCommandBuffer.bindDescriptorSets(pRayTracingPipelineStateVulkan->GetVulkanPipelineBindPoint(), pRayTracingPipelineStateVulkan->GetVulkanPipelineLayout(), 0, m_CommandListData.m_DescriptorSets.GetCount(), m_CommandListData.m_DescriptorSets.GetData(), m_CommandListData.m_DynamicUniformBufferOffsets.GetCount(), m_CommandListData.m_DynamicUniformBufferOffsets.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
      }
    }

    m_CommandListData.m_bDescriptorsModified = false;
  }

  if (mode == xiiGALStateTransitionMode::Transition)
  {
    FlushBarriers();
  }

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  xiiSharedPtr<xiiGALDeviceVulkan>            pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTextureViewVulkan*                    pTextureViewVulkan = xiiDynamicCast<xiiGALTextureViewVulkan*>(pRenderTargetView);
  xiiGALTextureVulkan*                        pTextureVulkan     = pTextureViewVulkan->GetTexture().Downcast<xiiGALTextureVulkan>();
  const xiiGALTextureViewCreationDescription& viewDescription    = pTextureViewVulkan->GetDescription();

  // Check if the texture is one of the currently bound render targets.
  xiiUInt32 uiAttachmentIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_CommandListData.m_uiBoundRenderTargetCount; ++i)
  {
    if (m_CommandListData.m_pBoundRenderTargets[i] == pTextureViewVulkan)
    {
      uiAttachmentIndex = i;
      break;
    }
  }

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE || uiAttachmentIndex != xiiInvalidIndex, "Render target was not found in the frame buffer. This is unexpected because the render pass should either be invalid or the render target view should be part of an active render pass.");

  if (uiAttachmentIndex != xiiInvalidIndex)
  {
    XII_ASSERT_DEV(m_pRenderPass != nullptr && m_pFramebuffer != nullptr, "The render pass or frame buffer is invalid while the texture was bound.");

    // The aspectMask of all image subresource ranges must only include VK_IMAGE_ASPECT_COLOR_BIT(17.1)

    vk::ClearAttachment vkClearAttachment = {};
    vkClearAttachment.aspectMask          = vk::ImageAspectFlagBits::eColor;
    vkClearAttachment.clearValue.color    = ClearValueToVulkanClearValue(clearColor.GetData(), viewDescription.m_Format);

    // colorAttachment is only meaningful if VK_IMAGE_ASPECT_COLOR_BIT is set in aspectMask, in which case it is an index to the pColorAttachments array in the VkSubpassDescription
    // structure of the current subpass which selects the color attachment to clear (17.2).
    // It is NOT the render pass attachment index.
    vkClearAttachment.colorAttachment = uiAttachmentIndex;

    vk::ClearRect vkClearRect  = {};
    vkClearRect.rect           = vk::Rect2D{{0, 0}, {m_CommandListState.m_uiFramebufferWidth, m_CommandListState.m_uiFramebufferHeight}}; // m_uiFramebufferWidth, m_uiFramebufferHeight are scaled to the proper mip level.
    vkClearRect.baseArrayLayer = 0;                                                                                                       // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice.
    vkClearRect.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;

    // No memory barriers are needed between vkCmdClearAttachments and preceding or subsequent draw or attachment clear commands in the same subpass (17.2)
    m_vkCommandBuffer.clearAttachments(1U, &vkClearAttachment, 1U, &vkClearRect, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdClearColorImage() must be called outside render pass (17.1)");

    // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
    TransitionOrVerifyTextureState(pTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Clearing render target outside of render pass.");

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
  xiiSharedPtr<xiiGALDeviceVulkan>            pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTextureViewVulkan*                    pTextureViewVulkan = xiiDynamicCast<xiiGALTextureViewVulkan*>(pDepthStencilView);
  xiiGALTextureVulkan*                        pTextureVulkan     = xiiDynamicCast<xiiGALTextureVulkan*>(pDepthStencilView->GetTexture());
  const xiiGALTextureViewCreationDescription& viewDescription    = pDepthStencilView->GetDescription();

  XII_ASSERT_DEV(viewDescription.m_ResourceDimension != xiiGALResourceDimension::Texture3D, "Depth-stencil view of a 3D texture must be created as a 2D texture array view.");

  const bool bClearAsAttachment = m_CommandListData.m_pBoundDepthStencilTarget == pTextureViewVulkan;
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE || bClearAsAttachment, "Depth-stencil view was not found in the frame buffer. This is unexpected because the render pass should either be invalid or the depth stencil view should be part of an active render pass.");

  if (bClearAsAttachment)
  {
    XII_ASSERT_DEV(m_pRenderPass != nullptr && m_pFramebuffer != nullptr, "The render pass or frame buffer is invalid while the texture was bound.");

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
    vkClearRect.rect           = vk::Rect2D{{0, 0}, {m_CommandListState.m_uiFramebufferWidth, m_CommandListState.m_uiFramebufferHeight}}; // m_uiFramebufferWidth, m_uiFramebufferHeight are scaled to the proper mip level.
    vkClearRect.baseArrayLayer = 0;                                                                                                       // The layers [baseArrayLayer, baseArrayLayer + layerCount) count from the base layer of the attachment image view (17.2), so baseArrayLayer is 0, not ViewDesc.FirstArraySlice.
    vkClearRect.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;

    // No memory barriers are needed between vkCmdClearAttachments and preceding or subsequent draw or attachment clear commands in the same subpass (17.2)
    m_vkCommandBuffer.clearAttachments(1U, &vkClearAttachment, 1U, &vkClearRect, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdClearDepthStencilImage() must be called outside render pass (17.1)");

    // Image layout must be VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (17.1)
    TransitionOrVerifyTextureState(pTextureVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::ImageLayout::eTransferDstOptimal, "Clearing depth-stencil outside of render pass.");

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
  xiiSharedPtr<xiiGALDeviceVulkan>            pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALRenderPassVulkan*                     pRenderPassVulkan      = xiiDynamicCast<xiiGALRenderPassVulkan*>(pRenderPass);
  xiiGALFramebufferVulkan*                    pFramebufferVulkan     = xiiDynamicCast<xiiGALFramebufferVulkan*>(pFramebuffer);
  const xiiGALRenderPassCreationDescription&  renderPassDescription  = pRenderPassVulkan->GetDescription();
  const xiiGALFramebufferCreationDescription& framebufferDescription = pFramebufferVulkan->GetDescription();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Current render pass has not yet been ended.");

  if (m_CommandListState.m_vkRenderPass != pRenderPassVulkan->GetVulkanRenderPass() || m_CommandListState.m_vkFramebuffer != pFramebufferVulkan->GetVulkanFramebuffer())
  {
    for (xiiUInt32 i = 0; i < renderPassDescription.m_Attachments.GetCount(); ++i)
    {
      const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[i];
      const xiiSharedPtr<xiiGALTextureView>&       pAttachmentView       = framebufferDescription.m_Attachments[i];

      xiiGALTextureVulkan* pTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(pAttachmentView->GetTexture());

      if (pTextureVulkan->IsInKnownState() && !pTextureVulkan->CheckState((xiiGALResourceStateFlags::Enum)attachmentDescription.m_InitialStateFlags.GetValue()))
      {
        TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, attachmentDescription.m_InitialStateFlags, xiiGALStateTransitionFlags::UpdateState);
      }
    }

    FlushBarriers();

    xiiTemporaryHybridArray<vk::ClearValue, 8U> clearColorValues;

    for (xiiUInt32 i = 0; i < xiiMath::Min(renderPassDescription.m_Attachments.GetCount(), pOptimizedClearValues.GetCount()); ++i)
    {
      const xiiGALOptimizedClearValue& clearValue   = pOptimizedClearValues[i];
      vk::ClearValue                   vkClearValue = {};

      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(renderPassDescription.m_Attachments[i].m_Format);

      if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth || formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
      {
        vkClearValue.depthStencil.depth   = clearValue.m_DepthStencil.m_fDepth;
        vkClearValue.depthStencil.stencil = clearValue.m_DepthStencil.m_uiStencil;
      }
      else
      {
        vkClearValue.color.float32[0] = clearValue.m_ClearColour.r;
        vkClearValue.color.float32[1] = clearValue.m_ClearColour.g;
        vkClearValue.color.float32[2] = clearValue.m_ClearColour.b;
        vkClearValue.color.float32[3] = clearValue.m_ClearColour.a;
      }

      clearColorValues.PushBack(vkClearValue);
    }

    vk::RenderPassBeginInfo vkRenderPassBeginInfo = {};
    vkRenderPassBeginInfo.pNext                   = nullptr;
    vkRenderPassBeginInfo.renderPass              = pRenderPassVulkan->GetVulkanRenderPass();
    vkRenderPassBeginInfo.framebuffer             = pFramebufferVulkan->GetVulkanFramebuffer();
    vkRenderPassBeginInfo.renderArea              = vk::Rect2D{{0, 0}, {framebufferDescription.m_FramebufferSize.width, framebufferDescription.m_FramebufferSize.height}}; // The render area MUST be contained within the framebuffer dimensions (7.4)
    vkRenderPassBeginInfo.clearValueCount         = clearColorValues.GetCount();
    vkRenderPassBeginInfo.pClearValues            = clearColorValues.GetData(); // An array of VkClearValue structures that contains clear values for each attachment, if the attachment uses a loadOp value of VK_ATTACHMENT_LOAD_OP_CLEAR
                                                                                // or if the attachment has a depth/stencil format and uses a stencilLoadOp value of VK_ATTACHMENT_LOAD_OP_CLEAR. The array is indexed by attachment number. Only elements
                                                                                // corresponding to cleared attachments are used. Other elements of pClearValues are  ignored (7.4)

    // The contents of the subpass will be recorded inline in the primary command buffer, and secondary command buffers must not be executed within the subpass.
    m_vkCommandBuffer.beginRenderPass(&vkRenderPassBeginInfo, vk::SubpassContents::eInline, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_CommandListState.m_vkRenderPass             = pRenderPassVulkan->GetVulkanRenderPass();
    m_CommandListState.m_vkFramebuffer            = pFramebufferVulkan->GetVulkanFramebuffer();
    m_CommandListState.m_uiFramebufferWidth       = framebufferDescription.m_FramebufferSize.width;
    m_CommandListState.m_uiFramebufferHeight      = framebufferDescription.m_FramebufferSize.height;
    m_CommandListState.m_uiFramebufferArraySlices = framebufferDescription.m_uiArraySliceCount;
    m_CommandListState.m_bIsShadingRateSet        = false;
  }

  // m_bShadingRateIsSet = false;
}

void xiiGALCommandListVulkan::NextSubpassPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "Render pass has not yet been started.");

  m_vkCommandBuffer.nextSubpass(vk::SubpassContents::eInline, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::EndRenderPassPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "Render pass has not yet been started.");

  m_vkCommandBuffer.endRenderPass(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_CommandListState.m_vkRenderPass        = VK_NULL_HANDLE;
  m_CommandListState.m_vkFramebuffer       = VK_NULL_HANDLE;
  m_CommandListState.m_uiFramebufferWidth  = 0U;
  m_CommandListState.m_uiFramebufferHeight = 0U;

  if (m_CommandListState.m_uiInsidePassQueries != 0)
  {
    xiiLog::Error("Ending render pass while there are outstanding queries that have been started inside the pass, but have not been ended. Vulkan requires that a query must either begin and end inside the same "
                  "subpass of a render pass instance, or must both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)");
  }
}

void xiiGALCommandListVulkan::DrawPlatform(const xiiGALDrawDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDraw() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForDraw();

  if (description.m_uiVertexCount > 0 && description.m_uiInstanceCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    m_vkCommandBuffer.draw(description.m_uiVertexCount, description.m_uiInstanceCount, description.m_uiStartVertexLocation, description.m_uiFirstInstanceLocation, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::DrawIndexedPlatform(const xiiGALDrawIndexedDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexed() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForIndexedDraw(description.m_IndexType);

  if (description.m_uiIndexCount > 0 && description.m_uiInstanceCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    m_vkCommandBuffer.drawIndexed(description.m_uiIndexCount, description.m_uiInstanceCount, description.m_uiFirstIndexLocation, description.m_uiBaseVertex, description.m_uiFirstInstanceLocation, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::DrawIndirectPlatform(const xiiGALDrawIndirectDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndirect() or vkCmdDrawIndirectCount() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForDraw();

  if (description.m_uiDrawCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    xiiGALBufferVulkan*              pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pBuffer);

    if (description.m_pCounterBuffer == nullptr)
    {
      m_vkCommandBuffer.drawIndirect(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, description.m_uiDrawCount, (description.m_uiDrawCount > 1 ? description.m_uiDrawArgumentStride : 0U), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
    else
    {
      xiiGALBufferVulkan* pCounterBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pCounterBuffer);

      m_vkCommandBuffer.drawIndirectCount(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, pCounterBufferVulkan->GetVulkanBuffer(), description.m_uiCounterOffset, description.m_uiDrawCount, description.m_uiDrawArgumentStride, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
  }
}

void xiiGALCommandListVulkan::DrawIndexedIndirectPlatform(const xiiGALDrawIndexedIndirectDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawIndexedindirect() or vkCmdDrawIndexedindirectCount() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForIndexedDraw(description.m_IndexType);

  if (description.m_uiDrawCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    xiiGALBufferVulkan*              pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pBuffer);

    if (description.m_pCounterBuffer == nullptr)
    {
      m_vkCommandBuffer.drawIndexedIndirect(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, description.m_uiDrawCount, (description.m_uiDrawCount > 1 ? description.m_uiDrawArgumentStride : 0U), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
    else
    {
      xiiGALBufferVulkan* pCounterBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pCounterBuffer);

      m_vkCommandBuffer.drawIndexedIndirectCount(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, pCounterBufferVulkan->GetVulkanBuffer(), description.m_uiCounterOffset, description.m_uiDrawCount, description.m_uiDrawArgumentStride, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
  }
}

void xiiGALCommandListVulkan::DrawMeshPlatform(const xiiGALDrawMeshDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawMeshTasksEXT() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForDraw();

  if (description.m_uiThreadGroupCountX > 0 && description.m_uiThreadGroupCountY > 0 && description.m_uiThreadGroupCountZ > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    m_vkCommandBuffer.drawMeshTasksEXT(description.m_uiThreadGroupCountX, description.m_uiThreadGroupCountY, description.m_uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::DrawMeshIndirectPlatform(const xiiGALDrawMeshIndirectDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawMeshTasksIndirectEXT() or vkCmdDrawMeshTasksIndirectCountEXT() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForDraw();

  if (description.m_uiCommandCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    xiiGALBufferVulkan*              pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pBuffer);

    if (description.m_pCounterBuffer == nullptr)
    {
      m_vkCommandBuffer.drawMeshTasksIndirectEXT(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, description.m_uiCommandCount, s_uiDrawMeshIndirectCommandStride, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
    else
    {
      xiiGALBufferVulkan* pCounterBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pCounterBuffer);

      m_vkCommandBuffer.drawMeshTasksIndirectCountEXT(pBufferVulkan->GetVulkanBuffer(), description.m_uiDrawArgumentOffset, pCounterBufferVulkan->GetVulkanBuffer(), description.m_uiCounterOffset, description.m_uiCommandCount, s_uiDrawMeshIndirectCommandStride, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
  }
}

void xiiGALCommandListVulkan::MultiDrawPlatform(const xiiGALMultiDrawDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawMultiEXT() or vkCmdDraw() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForDraw();

  if (description.m_uiInstanceCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    if (m_bNativeMultiDrawSupported)
    {
      xiiTemporaryHybridArray<vk::MultiDrawInfoEXT, 4U> multiDrawItems;
      multiDrawItems.SetCountUninitialized(description.m_pDrawItems.GetCount());

      for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
      {
        const xiiGALMultiDrawItem& drawItem = description.m_pDrawItems[i];

        if (drawItem.m_uiVertexCount > 0)
        {
          multiDrawItems[i].firstVertex = drawItem.m_uiStartVertexLocation;
          multiDrawItems[i].vertexCount = drawItem.m_uiVertexCount;
        }
      }

      if (!multiDrawItems.IsEmpty())
      {
        m_vkCommandBuffer.drawMultiEXT(multiDrawItems.GetCount(), multiDrawItems.GetData(), description.m_uiInstanceCount, description.m_uiFirstInstanceLocation, sizeof(vk::MultiDrawInfoEXT), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
      {
        const xiiGALMultiDrawItem& drawItem = description.m_pDrawItems[i];

        if (drawItem.m_uiVertexCount > 0)
        {
          m_vkCommandBuffer.draw(drawItem.m_uiVertexCount, description.m_uiInstanceCount, drawItem.m_uiStartVertexLocation, description.m_uiFirstInstanceLocation, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
      }
    }
  }
}

void xiiGALCommandListVulkan::MultiDrawIndexedPlatform(const xiiGALMultiDrawIndexedDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE, "vkCmdDrawMultiIndexedEXT() must be called inside render pass. (19.3)");
  XII_ASSERT_DEV(m_CommandListState.m_vkGraphicsPipeline != VK_NULL_HANDLE, "No graphics pipeline bound.");

  PrepareForIndexedDraw(description.m_IndexType);

  if (description.m_uiInstanceCount > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    if (m_bNativeMultiDrawSupported)
    {
      xiiTemporaryHybridArray<vk::MultiDrawIndexedInfoEXT, 4U> multiDrawIndexedItems;
      multiDrawIndexedItems.SetCountUninitialized(description.m_pDrawItems.GetCount());

      for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
      {
        const xiiGALMultiDrawIndexedItem& drawItem = description.m_pDrawItems[i];

        if (drawItem.m_uiIndexCount > 0)
        {
          multiDrawIndexedItems[i].firstIndex   = drawItem.m_uiFirstIndexLocation;
          multiDrawIndexedItems[i].indexCount   = drawItem.m_uiIndexCount;
          multiDrawIndexedItems[i].vertexOffset = static_cast<xiiInt32>(drawItem.m_uiBaseVertex);
        }
      }

      if (!multiDrawIndexedItems.IsEmpty())
      {
        // NULL or a pointer to the value added to the vertex index before indexing into the vertex buffer.
        // When specified, vk::MultiDrawIndexedInfoEXT::vertexOffset is ignored.
        static constexpr xiiInt32* pVertexOffset = nullptr;

        m_vkCommandBuffer.drawMultiIndexedEXT(multiDrawIndexedItems.GetCount(), multiDrawIndexedItems.GetData(), description.m_uiInstanceCount, description.m_uiFirstInstanceLocation, sizeof(vk::MultiDrawIndexedInfoEXT), pVertexOffset, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
      {
        const xiiGALMultiDrawIndexedItem& drawItem = description.m_pDrawItems[i];

        if (drawItem.m_uiIndexCount > 0)
        {
          m_vkCommandBuffer.drawIndexed(drawItem.m_uiIndexCount, description.m_uiInstanceCount, drawItem.m_uiFirstIndexLocation, drawItem.m_uiBaseVertex, description.m_uiFirstInstanceLocation, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        }
      }
    }
  }
}

void xiiGALCommandListVulkan::DispatchComputePlatform(const xiiGALDispatchComputeDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdDispatch() must be called outside of render pass. (27)");
  XII_ASSERT_DEV(m_CommandListState.m_vkComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound.");

  PrepareForDispatchCompute();

  if (description.m_uiThreadGroupCountX > 0 && description.m_uiThreadGroupCountY > 0 && description.m_uiThreadGroupCountZ > 0)
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    m_vkCommandBuffer.dispatch(description.m_uiThreadGroupCountX, description.m_uiThreadGroupCountY, description.m_uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::DispatchComputeIndirectPlatform(const xiiGALDispatchComputeIndirectDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdDispatchIndirect() must be called outside of render pass. (27)");
  XII_ASSERT_DEV(m_CommandListState.m_vkComputePipeline != VK_NULL_HANDLE, "No compute pipeline bound.");

  PrepareForDispatchCompute();

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pBuffer);

  m_vkCommandBuffer.dispatchIndirect(pBufferVulkan->GetVulkanBuffer(), description.m_uiDispatchArgumentOffset, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::TraceRaysPlatform(const xiiGALTraceRaysDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdTraceRaysKHR() must be called outside of render pass.");
  XII_ASSERT_DEV(m_CommandListState.m_vkRayTracingPipeline != VK_NULL_HANDLE, "No ray tracing pipeline bound.");

  PrepareForRayTracing();

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pSBTBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pShaderBindingTable);

  TransitionOrVerifyBufferState(pSBTBufferVulkan, description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::RayTracing, vk::AccessFlagBits::eShaderRead, "Binding shader binding table for ray tracing dispatch");

  auto BuildRegion = [&](const xiiGALRayTracingSBTRegionDescription& region) -> vk::StridedDeviceAddressRegionKHR {
    vk::StridedDeviceAddressRegionKHR vkRegion = {};

    if (region.m_uiSize == 0U)
      return vkRegion;

    vkRegion.deviceAddress = pSBTBufferVulkan->GetVulkanBufferDeviceAddress() + region.m_uiOffset;
    vkRegion.size          = region.m_uiSize;
    vkRegion.stride        = region.m_uiStride;
    return vkRegion;
  };

  vk::StridedDeviceAddressRegionKHR vkRayGenerationRegion = BuildRegion(description.m_RayGenerationTable);
  vk::StridedDeviceAddressRegionKHR vkMissRegion          = BuildRegion(description.m_MissTable);
  vk::StridedDeviceAddressRegionKHR vkHitRegion           = BuildRegion(description.m_HitTable);
  vk::StridedDeviceAddressRegionKHR vkCallableRegion      = BuildRegion(description.m_CallableTable);

  FlushBarriers();

  if (description.m_uiWidth > 0U && description.m_uiHeight > 0U && description.m_uiDepth > 0U)
  {
    m_vkCommandBuffer.traceRaysKHR(&vkRayGenerationRegion, &vkMissRegion, &vkHitRegion, &vkCallableRegion, description.m_uiWidth, description.m_uiHeight, description.m_uiDepth, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::TraceRaysIndirectPlatform(const xiiGALTraceRaysIndirectDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdTraceRaysIndirectKHR() must be called outside of render pass.");
  XII_ASSERT_DEV(m_CommandListState.m_vkRayTracingPipeline != VK_NULL_HANDLE, "No ray tracing pipeline bound.");

  PrepareForRayTracing();

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan         = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pSBTBufferVulkan      = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pShaderBindingTable);
  xiiGALBufferVulkan*              pArgumentBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pArgumentBuffer);

  TransitionOrVerifyBufferState(pSBTBufferVulkan, description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::RayTracing, vk::AccessFlagBits::eShaderRead, "Binding shader binding table for indirect ray tracing dispatch");
  TransitionOrVerifyBufferState(pArgumentBufferVulkan, description.m_ArgumentBufferTransitionMode, xiiGALResourceStateFlags::IndirectArgument, vk::AccessFlagBits::eIndirectCommandRead, "Binding indirect ray tracing argument buffer");

  auto BuildRegion = [&](const xiiGALRayTracingSBTRegionDescription& region) -> vk::StridedDeviceAddressRegionKHR {
    vk::StridedDeviceAddressRegionKHR vkRegion = {};

    if (region.m_uiSize == 0U)
      return vkRegion;

    vkRegion.deviceAddress = pSBTBufferVulkan->GetVulkanBufferDeviceAddress() + region.m_uiOffset;
    vkRegion.size          = region.m_uiSize;
    vkRegion.stride        = region.m_uiStride;
    return vkRegion;
  };

  vk::StridedDeviceAddressRegionKHR vkRayGenerationRegion = BuildRegion(description.m_RayGenerationTable);
  vk::StridedDeviceAddressRegionKHR vkMissRegion          = BuildRegion(description.m_MissTable);
  vk::StridedDeviceAddressRegionKHR vkHitRegion           = BuildRegion(description.m_HitTable);
  vk::StridedDeviceAddressRegionKHR vkCallableRegion      = BuildRegion(description.m_CallableTable);

  FlushBarriers();

  const vk::DeviceAddress vkIndirectAddress = pArgumentBufferVulkan->GetVulkanBufferDeviceAddress() + description.m_uiArgumentOffset;
  m_vkCommandBuffer.traceRaysIndirectKHR(&vkRayGenerationRegion, &vkMissRegion, &vkHitRegion, &vkCallableRegion, vkIndirectAddress, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::UpdateSBTPlatform(const xiiGALUpdateSBTDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pSBTBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pShaderBindingTable);

  xiiGALRayTracingPipelineState* pPipelineState = description.m_pPipelineState;
  if (pPipelineState == nullptr)
  {
    pPipelineState = xiiDynamicCast<xiiGALRayTracingPipelineState*>(m_pPipelineState);
  }

  XII_ASSERT_DEV(pPipelineState != nullptr, "UpdateSBT requires a valid ray tracing pipeline state.");

  xiiGALRayTracingPipelineStateVulkan* pRayTracingPipelineStateVulkan = xiiDynamicCast<xiiGALRayTracingPipelineStateVulkan*>(pPipelineState);
  XII_ASSERT_DEV(pRayTracingPipelineStateVulkan != nullptr, "UpdateSBT requires a Vulkan ray tracing pipeline state.");

  const xiiGALRayTracingProperties& rayTracingProperties = pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt32                   uiHandleSize         = rayTracingProperties.m_uiShaderGroupHandleSize;
  XII_ASSERT_DEV(uiHandleSize > 0U, "UpdateSBT failed: shader group handle size is zero.");

  xiiArrayPtr<const xiiUInt8> shaderGroupHandles = pRayTracingPipelineStateVulkan->GetShaderGroupHandles();
  XII_ASSERT_DEV(!shaderGroupHandles.IsEmpty(), "UpdateSBT failed: pipeline does not have cached shader group handles.");

  const xiiUInt32 uiGroupCount = static_cast<xiiUInt32>(shaderGroupHandles.GetCount() / uiHandleSize);

  struct RecordWrite
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiDestinationOffset = 0U;
    xiiUInt32 m_uiGroupIndex        = 0U;
  };

  xiiTemporaryHybridArray<RecordWrite, 4U> recordWrites;

  auto CollectRecordWrites = [&](const xiiGALRayTracingSBTRegionDescription& region, xiiArrayPtr<const xiiUInt32> groupIndices, xiiUInt32 uiStartIndex, const char* szRegionName) {
    if (region.m_uiSize == 0U)
      return;

    XII_ASSERT_DEV(region.m_uiStride >= uiHandleSize, "UpdateSBT {} region stride ({}) must be at least shader group handle size ({}).", szRegionName, region.m_uiStride, uiHandleSize);
    XII_ASSERT_DEV(region.m_uiStride > 0U, "UpdateSBT {} region stride must be non-zero.", szRegionName);

    const xiiUInt32 uiRecordCount = static_cast<xiiUInt32>(region.m_uiSize / region.m_uiStride);
    XII_ASSERT_DEV((uiStartIndex + uiRecordCount) <= groupIndices.GetCount(), "UpdateSBT {} region requested {} records starting at {}, but only {} shader groups are available.", szRegionName, uiRecordCount, uiStartIndex, groupIndices.GetCount());

    for (xiiUInt32 i = 0U; i < uiRecordCount; ++i)
    {
      const xiiUInt32 uiGroupIndex = groupIndices[uiStartIndex + i];
      XII_ASSERT_DEV(uiGroupIndex < uiGroupCount, "UpdateSBT {} region resolved an invalid shader group index {} (group count {}).", szRegionName, uiGroupIndex, uiGroupCount);

      RecordWrite& write          = recordWrites.ExpandAndGetRef();
      write.m_uiDestinationOffset = region.m_uiOffset + static_cast<xiiUInt64>(i) * region.m_uiStride;
      write.m_uiGroupIndex        = uiGroupIndex;
    }
  };

  CollectRecordWrites(description.m_RayGenerationTable, pRayTracingPipelineStateVulkan->GetRayGenerationGroupIndices(), description.m_uiRayGenerationShaderStartIndex, "RayGeneration");
  CollectRecordWrites(description.m_MissTable, pRayTracingPipelineStateVulkan->GetMissGroupIndices(), description.m_uiMissShaderStartIndex, "Miss");
  CollectRecordWrites(description.m_HitTable, pRayTracingPipelineStateVulkan->GetHitGroupIndices(), description.m_uiHitGroupStartIndex, "Hit");
  CollectRecordWrites(description.m_CallableTable, pRayTracingPipelineStateVulkan->GetCallableGroupIndices(), description.m_uiCallableShaderStartIndex, "Callable");

  if (recordWrites.IsEmpty())
    return;

  const xiiUInt64 uiUploadSize = static_cast<xiiUInt64>(recordWrites.GetCount()) * uiHandleSize;

  xiiVulkanMemoryAllocator*           pVulkanMemoryAllocator  = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALStagingBufferAllocationVulkan stagingBufferAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(uiUploadSize);

  void* pMappedMemory = nullptr;
  VK_ASSERT_DEV(pVulkanMemoryAllocator->MapMemory(stagingBufferAllocation.m_VulkanAllocation, &pMappedMemory));
  VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, uiUploadSize));

  pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);

  xiiTemporaryHybridArray<vk::BufferCopy, 4U> vkCopyRegions;
  vkCopyRegions.SetCountUninitialized(recordWrites.GetCount());

  for (xiiUInt32 i = 0U; i < recordWrites.GetCount(); ++i)
  {
    const RecordWrite& write = recordWrites[i];

    const xiiUInt64 uiSourceOffset = static_cast<xiiUInt64>(write.m_uiGroupIndex) * uiHandleSize;
    XII_ASSERT_DEV((uiSourceOffset + uiHandleSize) <= shaderGroupHandles.GetCount(), "UpdateSBT resolved shader group handle range out of bounds.");

    xiiMemoryUtils::RawByteCopy(xiiMemoryUtils::AddByteOffset(pMappedMemory, static_cast<xiiUInt64>(i) * uiHandleSize), shaderGroupHandles.GetPtr() + uiSourceOffset, uiHandleSize);

    vk::BufferCopy& vkCopyRegion = vkCopyRegions[i];
    vkCopyRegion.srcOffset       = stagingBufferAllocation.m_uiOffset + static_cast<xiiUInt64>(i) * uiHandleSize;
    vkCopyRegion.dstOffset       = write.m_uiDestinationOffset;
    vkCopyRegion.size            = uiHandleSize;
  }

  VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, uiUploadSize));
  pVulkanMemoryAllocator->UnmapMemory(stagingBufferAllocation.m_VulkanAllocation);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");
  TransitionOrVerifyBufferState(pSBTBufferVulkan, description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Updating shader binding table");

  FlushBarriers();

  m_vkCommandBuffer.copyBuffer(stagingBufferAllocation.m_vkBuffer, pSBTBufferVulkan->GetVulkanBuffer(), vkCopyRegions.GetCount(), vkCopyRegions.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::BuildBLASPlatform(const xiiGALBuildBLASDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdBuildAccelerationStructuresKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan>              pDeviceVulkan        = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBottomLevelASVulkan*                    pBottomLevelASVulkan = xiiDynamicCast<xiiGALBottomLevelASVulkan*>(description.m_pBottomLevelAS);
  xiiGALBufferVulkan*                           pScratchBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pScratchBuffer);
  const xiiGALBottomLevelASCreationDescription& blasDescription      = description.m_pBottomLevelAS->GetDescription();

  TransitionOrVerifyBufferState(pScratchBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR, "Using scratch buffer for BLAS build");

  xiiTemporaryHybridArray<vk::AccelerationStructureGeometryKHR, 4U>              vkGeometries;
  xiiTemporaryHybridArray<vk::AccelerationStructureBuildRangeInfoKHR, 4U>        vkBuildRanges;
  xiiTemporaryHybridArray<const vk::AccelerationStructureBuildRangeInfoKHR*, 4U> vkBuildRangePointers;

  const xiiUInt32 uiTotalGeometryCount = blasDescription.m_Triangles.GetCount() + blasDescription.m_BoundingBoxes.GetCount();
  vkGeometries.Reserve(uiTotalGeometryCount);
  vkBuildRanges.Reserve(uiTotalGeometryCount);
  vkBuildRangePointers.Reserve(uiTotalGeometryCount);

  for (xiiUInt32 i = 0U; i < blasDescription.m_Triangles.GetCount(); ++i)
  {
    const xiiGALBLASTriangleDescription&      triangleDescription = blasDescription.m_Triangles[i];
    const xiiGALBLASTriangleBuildDescription& triangleBuildData   = description.m_Triangles[i];

    xiiGALBufferVulkan* pVertexBufferVulkan    = xiiDynamicCast<xiiGALBufferVulkan*>(triangleBuildData.m_pVertexBuffer);
    xiiGALBufferVulkan* pIndexBufferVulkan     = xiiDynamicCast<xiiGALBufferVulkan*>(triangleBuildData.m_pIndexBuffer);
    xiiGALBufferVulkan* pTransformBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(triangleBuildData.m_pTransformBuffer);

    TransitionOrVerifyBufferState(pVertexBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, vk::AccessFlagBits::eAccelerationStructureReadKHR, "Using vertex buffer for BLAS build");
    if (pIndexBufferVulkan != nullptr)
    {
      TransitionOrVerifyBufferState(pIndexBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, vk::AccessFlagBits::eAccelerationStructureReadKHR, "Using index buffer for BLAS build");
    }
    if (pTransformBufferVulkan != nullptr)
    {
      TransitionOrVerifyBufferState(pTransformBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, vk::AccessFlagBits::eAccelerationStructureReadKHR, "Using transform buffer for BLAS build");
    }

    vk::AccelerationStructureGeometryTrianglesDataKHR vkTriangleData = {};
    vkTriangleData.vertexFormat                                      = ConvertTriangleVertexFormat(triangleDescription);
    vkTriangleData.vertexData.deviceAddress                          = pVertexBufferVulkan->GetVulkanBufferDeviceAddress() + triangleBuildData.m_uiVertexBufferOffset;
    vkTriangleData.vertexStride                                      = triangleBuildData.m_uiVertexStride;
    vkTriangleData.maxVertex                                         = triangleDescription.m_uiMaxVertexCount;
    vkTriangleData.indexType                                         = triangleDescription.m_IndexType == xiiGALValueType::UInt16 ? vk::IndexType::eUint16 : (triangleDescription.m_IndexType == xiiGALValueType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eNoneKHR);
    vkTriangleData.indexData.deviceAddress                           = pIndexBufferVulkan != nullptr ? (pIndexBufferVulkan->GetVulkanBufferDeviceAddress() + triangleBuildData.m_uiIndexBufferOffset) : 0U;
    vkTriangleData.transformData.deviceAddress                       = pTransformBufferVulkan != nullptr ? (pTransformBufferVulkan->GetVulkanBufferDeviceAddress() + triangleBuildData.m_uiTransformOffset) : 0U;

    vk::AccelerationStructureGeometryDataKHR vkGeometryData = {};
    vkGeometryData.triangles                                = vkTriangleData;

    vk::AccelerationStructureGeometryKHR vkGeometry = {};
    vkGeometry.geometryType                         = vk::GeometryTypeKHR::eTriangles;
    vkGeometry.geometry                             = vkGeometryData;
    vkGeometry.flags                                = vk::GeometryFlagBitsKHR::eOpaque;

    vkGeometries.PushBack(vkGeometry);

    vk::AccelerationStructureBuildRangeInfoKHR vkBuildRange = {};
    vkBuildRange.primitiveCount                             = triangleBuildData.m_uiPrimitiveCount != 0U ? triangleBuildData.m_uiPrimitiveCount : triangleDescription.m_uiMaxPrimitiveCount;
    vkBuildRange.primitiveOffset                            = 0U;
    vkBuildRange.firstVertex                                = 0U;
    vkBuildRange.transformOffset                            = 0U;

    vkBuildRanges.PushBack(vkBuildRange);
  }

  for (xiiUInt32 i = 0U; i < blasDescription.m_BoundingBoxes.GetCount(); ++i)
  {
    const xiiGALBLASBoundingBoxDescription&      boxDescription = blasDescription.m_BoundingBoxes[i];
    const xiiGALBLASBoundingBoxBuildDescription& boxBuildData   = description.m_BoundingBoxes[i];

    xiiGALBufferVulkan* pBoundingBoxBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(boxBuildData.m_pBoundingBoxBuffer);
    TransitionOrVerifyBufferState(pBoundingBoxBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, vk::AccessFlagBits::eAccelerationStructureReadKHR, "Using AABB buffer for BLAS build");

    vk::AccelerationStructureGeometryAabbsDataKHR vkAABBsData = {};
    vkAABBsData.data.deviceAddress                            = pBoundingBoxBufferVulkan->GetVulkanBufferDeviceAddress() + boxBuildData.m_uiBoundingBoxOffset;
    vkAABBsData.stride                                        = boxBuildData.m_uiBoundingBoxStride;

    vk::AccelerationStructureGeometryDataKHR vkGeometryData = {};
    vkGeometryData.aabbs                                    = vkAABBsData;

    vk::AccelerationStructureGeometryKHR vkGeometry = {};
    vkGeometry.geometryType                         = vk::GeometryTypeKHR::eAabbs;
    vkGeometry.geometry                             = vkGeometryData;
    vkGeometry.flags                                = vk::GeometryFlagBitsKHR::eOpaque;

    vkGeometries.PushBack(vkGeometry);

    vk::AccelerationStructureBuildRangeInfoKHR vkBuildRange = {};
    vkBuildRange.primitiveCount                             = boxBuildData.m_uiBoxCount != 0U ? boxBuildData.m_uiBoxCount : boxDescription.m_uiMaxBoxCount;
    vkBuildRange.primitiveOffset                            = 0U;
    vkBuildRange.firstVertex                                = 0U;
    vkBuildRange.transformOffset                            = 0U;

    vkBuildRanges.PushBack(vkBuildRange);
  }

  vk::AccelerationStructureBuildGeometryInfoKHR vkBuildInfo = {};
  vkBuildInfo.type                                          = vk::AccelerationStructureTypeKHR::eBottomLevel;
  vkBuildInfo.flags                                         = ConvertBuildASFlags(description.m_BuildFlags.IsAnyFlagSet() ? description.m_BuildFlags : blasDescription.m_BuildASFlags);
  vkBuildInfo.mode                                          = description.m_bUpdate ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;
  vkBuildInfo.srcAccelerationStructure                      = description.m_bUpdate ? pBottomLevelASVulkan->GetVulkanAccelerationStructure() : VK_NULL_HANDLE;
  vkBuildInfo.dstAccelerationStructure                      = pBottomLevelASVulkan->GetVulkanAccelerationStructure();
  vkBuildInfo.geometryCount                                 = vkGeometries.GetCount();
  vkBuildInfo.pGeometries                                   = vkGeometries.GetData();
  vkBuildInfo.scratchData.deviceAddress                     = pScratchBufferVulkan->GetVulkanBufferDeviceAddress() + description.m_uiScratchBufferOffset;

  xiiBitflags<xiiGALResourceStateFlags> oldState = pBottomLevelASVulkan->GetResourceState();
  if (oldState == xiiGALResourceStateFlags::Unknown)
    oldState = xiiGALResourceStateFlags::BuildASWrite;

  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASWrite), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASWrite));
  pBottomLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASWrite);

  vkBuildRangePointers.SetCount(vkBuildRanges.GetCount());
  for (xiiUInt32 i = 0U; i < vkBuildRanges.GetCount(); ++i)
  {
    vkBuildRangePointers[i] = &vkBuildRanges[i];
  }

  FlushBarriers();

  m_vkCommandBuffer.buildAccelerationStructuresKHR(1U, &vkBuildInfo, vkBuildRangePointers.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::BuildTLASPlatform(const xiiGALBuildTLASDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdBuildAccelerationStructuresKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan>           pDeviceVulkan         = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTopLevelASVulkan*                    pTopLevelASVulkan     = xiiDynamicCast<xiiGALTopLevelASVulkan*>(description.m_pTopLevelAS);
  xiiGALBufferVulkan*                        pInstanceBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pInstanceBuffer);
  xiiGALBufferVulkan*                        pScratchBufferVulkan  = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pScratchBuffer);
  const xiiGALTopLevelASCreationDescription& tlasDescription       = description.m_pTopLevelAS->GetDescription();

  TransitionOrVerifyBufferState(pInstanceBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, vk::AccessFlagBits::eAccelerationStructureReadKHR, "Using instance buffer for TLAS build");
  TransitionOrVerifyBufferState(pScratchBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR, "Using scratch buffer for TLAS build");

  vk::AccelerationStructureGeometryInstancesDataKHR vkInstancesData = {};
  vkInstancesData.arrayOfPointers                                   = vk::False;
  vkInstancesData.data.deviceAddress                                = pInstanceBufferVulkan->GetVulkanBufferDeviceAddress() + description.m_uiInstanceBufferOffset;

  vk::AccelerationStructureGeometryDataKHR vkGeometryData = {};
  vkGeometryData.instances                                = vkInstancesData;

  vk::AccelerationStructureGeometryKHR vkGeometry = {};
  vkGeometry.geometryType                         = vk::GeometryTypeKHR::eInstances;
  vkGeometry.geometry                             = vkGeometryData;

  vk::AccelerationStructureBuildRangeInfoKHR vkBuildRange = {};
  vkBuildRange.primitiveCount                             = description.m_uiInstanceCount;

  vk::AccelerationStructureBuildGeometryInfoKHR vkBuildInfo = {};
  vkBuildInfo.type                                          = vk::AccelerationStructureTypeKHR::eTopLevel;
  vkBuildInfo.flags                                         = ConvertBuildASFlags(description.m_BuildFlags.IsAnyFlagSet() ? description.m_BuildFlags : tlasDescription.m_Flags);
  vkBuildInfo.mode                                          = description.m_bUpdate ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;
  vkBuildInfo.srcAccelerationStructure                      = description.m_bUpdate ? pTopLevelASVulkan->GetVulkanAccelerationStructure() : VK_NULL_HANDLE;
  vkBuildInfo.dstAccelerationStructure                      = pTopLevelASVulkan->GetVulkanAccelerationStructure();
  vkBuildInfo.geometryCount                                 = 1U;
  vkBuildInfo.pGeometries                                   = &vkGeometry;
  vkBuildInfo.scratchData.deviceAddress                     = pScratchBufferVulkan->GetVulkanBufferDeviceAddress() + description.m_uiScratchBufferOffset;

  xiiBitflags<xiiGALResourceStateFlags> oldState = pTopLevelASVulkan->GetResourceState();
  if (oldState == xiiGALResourceStateFlags::Unknown)
  {
    oldState = xiiGALResourceStateFlags::BuildASWrite;
  }

  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASWrite), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASWrite));
  pTopLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASWrite);

  const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &vkBuildRange;

  FlushBarriers();

  m_vkCommandBuffer.buildAccelerationStructuresKHR(1U, &vkBuildInfo, &pRangeInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyBLASPlatform(const xiiGALCopyBLASDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdCopyAccelerationStructureKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan                   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBottomLevelASVulkan*       pSourceBottomLevelASVulkan      = xiiDynamicCast<xiiGALBottomLevelASVulkan*>(description.m_pSourceBottomLevelAS);
  xiiGALBottomLevelASVulkan*       pDestinationBottomLevelASVulkan = xiiDynamicCast<xiiGALBottomLevelASVulkan*>(description.m_pDestinationBottomLevelAS);

  xiiBitflags<xiiGALResourceStateFlags> sourceOldState = pSourceBottomLevelASVulkan->GetResourceState();
  if (sourceOldState == xiiGALResourceStateFlags::Unknown)
    sourceOldState = xiiGALResourceStateFlags::BuildASWrite;

  xiiBitflags<xiiGALResourceStateFlags> destinationOldState = pDestinationBottomLevelASVulkan->GetResourceState();
  if (destinationOldState == xiiGALResourceStateFlags::Unknown)
    destinationOldState = xiiGALResourceStateFlags::BuildASWrite;

  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(sourceOldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASRead), xiiVulkanTypeConversions::GetPipelineStageFlags(sourceOldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASRead));
  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(destinationOldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASWrite), xiiVulkanTypeConversions::GetPipelineStageFlags(destinationOldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASWrite));

  pSourceBottomLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASRead);
  pDestinationBottomLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASWrite);

  vk::CopyAccelerationStructureInfoKHR vkCopyInfo = {};
  vkCopyInfo.src                                  = pSourceBottomLevelASVulkan->GetVulkanAccelerationStructure();
  vkCopyInfo.dst                                  = pDestinationBottomLevelASVulkan->GetVulkanAccelerationStructure();
  vkCopyInfo.mode                                 = description.m_Mode == xiiGALASCopyMode::Compact ? vk::CopyAccelerationStructureModeKHR::eCompact : vk::CopyAccelerationStructureModeKHR::eClone;

  FlushBarriers();

  m_vkCommandBuffer.copyAccelerationStructureKHR(&vkCopyInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyTLASPlatform(const xiiGALCopyTLASDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdCopyAccelerationStructureKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan                = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTopLevelASVulkan*          pSourceTopLevelASVulkan      = xiiDynamicCast<xiiGALTopLevelASVulkan*>(description.m_pSourceTopLevelAS);
  xiiGALTopLevelASVulkan*          pDestinationTopLevelASVulkan = xiiDynamicCast<xiiGALTopLevelASVulkan*>(description.m_pDestinationTopLevelAS);

  xiiBitflags<xiiGALResourceStateFlags> sourceOldState = pSourceTopLevelASVulkan->GetResourceState();
  if (sourceOldState == xiiGALResourceStateFlags::Unknown)
    sourceOldState = xiiGALResourceStateFlags::BuildASWrite;

  xiiBitflags<xiiGALResourceStateFlags> destinationOldState = pDestinationTopLevelASVulkan->GetResourceState();
  if (destinationOldState == xiiGALResourceStateFlags::Unknown)
    destinationOldState = xiiGALResourceStateFlags::BuildASWrite;

  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(sourceOldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASRead), xiiVulkanTypeConversions::GetPipelineStageFlags(sourceOldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASRead));
  MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(destinationOldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASWrite), xiiVulkanTypeConversions::GetPipelineStageFlags(destinationOldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASWrite));

  pSourceTopLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASRead);
  pDestinationTopLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASWrite);

  vk::CopyAccelerationStructureInfoKHR vkCopyInfo = {};
  vkCopyInfo.src                                  = pSourceTopLevelASVulkan->GetVulkanAccelerationStructure();
  vkCopyInfo.dst                                  = pDestinationTopLevelASVulkan->GetVulkanAccelerationStructure();
  vkCopyInfo.mode                                 = description.m_Mode == xiiGALASCopyMode::Compact ? vk::CopyAccelerationStructureModeKHR::eCompact : vk::CopyAccelerationStructureModeKHR::eClone;

  FlushBarriers();

  m_vkCommandBuffer.copyAccelerationStructureKHR(&vkCopyInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::WriteBLASCompactedSizePlatform(const xiiGALWriteBLASCompactedSizeDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdWriteAccelerationStructuresPropertiesKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBottomLevelASVulkan*       pBottomLevelASVulkan     = xiiDynamicCast<xiiGALBottomLevelASVulkan*>(description.m_pBottomLevelAS);
  xiiGALBufferVulkan*              pDestinationBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pDestinationBuffer);

  TransitionOrVerifyBufferState(pDestinationBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Using compacted size destination buffer");

  xiiBitflags<xiiGALResourceStateFlags> oldState = pBottomLevelASVulkan->GetResourceState();
  if (oldState == xiiGALResourceStateFlags::Unknown)
    oldState = xiiGALResourceStateFlags::BuildASWrite;

  if (description.m_ResourceStateTransitionMode == xiiGALStateTransitionMode::Transition)
  {
    MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASRead), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASRead));
    pBottomLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASRead);
  }
  else if (description.m_ResourceStateTransitionMode == xiiGALStateTransitionMode::Verify)
  {
    XII_ASSERT_DEV(oldState.IsSet(xiiGALResourceStateFlags::BuildASRead), "Source BLAS ({}) is not in BuildASRead state while verifying compacted-size write.", pBottomLevelASVulkan->GetDebugName());
  }

  vk::QueryPool vkQueryPool = CreateCompactedSizeQueryPool(pDeviceVulkan.Borrow());
  m_CommandListData.m_TemporaryQueryPools.PushBack(vkQueryPool);

  const vk::AccelerationStructureKHR vkAccelerationStructure = pBottomLevelASVulkan->GetVulkanAccelerationStructure();

  FlushBarriers();

  m_vkCommandBuffer.resetQueryPool(vkQueryPool, 0U, 1U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_vkCommandBuffer.writeAccelerationStructuresPropertiesKHR(1U, &vkAccelerationStructure, vk::QueryType::eAccelerationStructureCompactedSizeKHR, vkQueryPool, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_vkCommandBuffer.copyQueryPoolResults(vkQueryPool, 0U, 1U, pDestinationBufferVulkan->GetVulkanBuffer(), description.m_uiDestinationBufferOffset, sizeof(xiiUInt64), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::WriteTLASCompactedSizePlatform(const xiiGALWriteTLASCompactedSizeDescription& description)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "vkCmdWriteAccelerationStructuresPropertiesKHR() must be called outside of render pass.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTopLevelASVulkan*          pTopLevelASVulkan        = xiiDynamicCast<xiiGALTopLevelASVulkan*>(description.m_pTopLevelAS);
  xiiGALBufferVulkan*              pDestinationBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(description.m_pDestinationBuffer);

  TransitionOrVerifyBufferState(pDestinationBufferVulkan, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Using compacted size destination buffer");

  xiiBitflags<xiiGALResourceStateFlags> oldState = pTopLevelASVulkan->GetResourceState();
  if (oldState == xiiGALResourceStateFlags::Unknown)
    oldState = xiiGALResourceStateFlags::BuildASWrite;

  if (description.m_ResourceStateTransitionMode == xiiGALStateTransitionMode::Transition)
  {
    MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(xiiGALResourceStateFlags::BuildASRead), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(xiiGALResourceStateFlags::BuildASRead));
    pTopLevelASVulkan->SetResourceState(xiiGALResourceStateFlags::BuildASRead);
  }
  else if (description.m_ResourceStateTransitionMode == xiiGALStateTransitionMode::Verify)
  {
    XII_ASSERT_DEV(oldState.IsSet(xiiGALResourceStateFlags::BuildASRead), "Source TLAS ({}) is not in BuildASRead state while verifying compacted-size write.", pTopLevelASVulkan->GetDebugName());
  }

  vk::QueryPool vkQueryPool = CreateCompactedSizeQueryPool(pDeviceVulkan.Borrow());
  m_CommandListData.m_TemporaryQueryPools.PushBack(vkQueryPool);

  const vk::AccelerationStructureKHR vkAccelerationStructure = pTopLevelASVulkan->GetVulkanAccelerationStructure();

  FlushBarriers();

  m_vkCommandBuffer.resetQueryPool(vkQueryPool, 0U, 1U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_vkCommandBuffer.writeAccelerationStructuresPropertiesKHR(1U, &vkAccelerationStructure, vk::QueryType::eAccelerationStructureCompactedSizeKHR, vkQueryPool, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_vkCommandBuffer.copyQueryPoolResults(vkQueryPool, 0U, 1U, pDestinationBufferVulkan->GetVulkanBuffer(), description.m_uiDestinationBufferOffset, sizeof(xiiUInt64), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALQueryPoolVulkan*           pQueryPoolVulkan = pDeviceVulkan->GetCommandQueueQueryPool(m_Description.m_QueueFlags);
  xiiGALQueryVulkan*               pQueryVulkan     = xiiDynamicCast<xiiGALQueryVulkan*>(pQuery);

  pQueryVulkan->OnBeginQuery(this);

  xiiGALQueryType::Enum queryType   = pQueryVulkan->GetDescription().m_Type;
  vk::QueryPool         vkQueryPool = pQueryPoolVulkan->GetQueryPool(queryType);
  xiiUInt32             uiIndex     = pQueryVulkan->GetQueryPoolIndex(0);

  XII_ASSERT_DEV(vkQueryPool != VK_NULL_HANDLE, "Query pool is not initialized for query type.");

  if (queryType == xiiGALQueryType::Timestamp)
  {
    xiiLog::Error("BeginQuery() is not supported for timestamp queries.");
  }
  else if (queryType == xiiGALQueryType::Duration)
  {
    m_vkCommandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, vkQueryPool, uiIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    if ((m_CommandListState.m_uiInsidePassQueries | m_CommandListState.m_uiOutsidePassQueries) & XII_BIT(queryType))
    {
      xiiLog::Error("Another query of type ({}) is currently active. Overlapping queries are not supported in Vulkan. End the first query before beginning another.", queryType);
      return;
    }

    // A query must either begin and end inside the same subpass of a render pass instance, or must both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)

    ++m_CommandListData.m_uiActiveQueriesCounter;

    // If flags does not contain VK_QUERY_CONTROL_PRECISE_BIT an implementation may generate any non-zero result value for the query if the count of passing samples is non-zero (17.3).

    // Query pool must have been created with a queryType that differs from that of any queries that are active within commandBuffer (17.2).
    // In other words, only one query of given type can be active in the command buffer.

    if ((m_CommandListState.m_uiInsidePassQueries | m_CommandListState.m_uiOutsidePassQueries) & XII_BIT(queryType))
    {
      xiiLog::Error("Another query of type ({}) is currently active. Overlapping queries are not supported in Vulkan. End the first query before beginning another.", queryType);
      return;
    }

    // A query must either begin and end inside the same subpass of a render pass instance, or must both begin and end outside a render pass instance (i.e. contain entire render pass instances) (17.2).

    m_vkCommandBuffer.beginQuery(vkQueryPool, uiIndex, (queryType == xiiGALQueryType::Occlusion ? vk::QueryControlFlagBits::ePrecise : vk::QueryControlFlags{}), pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
    {
      m_CommandListState.m_uiInsidePassQueries |= XII_BIT(queryType);
    }
    else
    {
      m_CommandListState.m_uiOutsidePassQueries |= XII_BIT(queryType);
    }
  }
}

void xiiGALCommandListVulkan::EndQueryPlatform(xiiGALQuery* pQuery)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan    = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALQueryPoolVulkan*           pQueryPoolVulkan = pDeviceVulkan->GetCommandQueueQueryPool(m_Description.m_QueueFlags);
  xiiGALQueryVulkan*               pQueryVulkan     = xiiDynamicCast<xiiGALQueryVulkan*>(pQuery);

  pQueryVulkan->OnEndQuery(this);

  xiiGALQueryType::Enum queryType   = pQueryVulkan->GetDescription().m_Type;
  vk::QueryPool         vkQueryPool = pQueryPoolVulkan->GetQueryPool(queryType);
  xiiUInt32             uiIndex     = pQueryVulkan->GetQueryPoolIndex(queryType == xiiGALQueryType::Duration ? 1 : 0);

  XII_ASSERT_DEV(vkQueryPool != VK_NULL_HANDLE, "Query pool is not initialized for query type.");

  if (queryType == xiiGALQueryType::Timestamp || queryType == xiiGALQueryType::Duration)
  {
    m_vkCommandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, vkQueryPool, uiIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  else
  {
    XII_ASSERT_DEV(m_CommandListData.m_uiActiveQueriesCounter > 0, "Active query counter is 0 which means there was a mismatch between BeginQuery() / EndQuery() calls");

    // A query must either begin and end inside the same subpass of a render pass instance, or must both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)

    XII_ASSERT_DEV((m_CommandListState.m_uiInsidePassQueries | m_CommandListState.m_uiOutsidePassQueries) & XII_BIT(queryType), "No query flag is set which indicates there was no matching BeginQuery call or there was an error while beginning the query.");

    if (m_CommandListState.m_uiOutsidePassQueries & XII_BIT(queryType))
    {
      if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
      {
        // Vulkan requires begin/end query to occur in the same render-pass scope.
        EndRenderPass();
      }
    }
    else
    {
      if (m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE)
      {
        xiiLog::Error("The query was started inside render pass, but is being ended outside of render pass. Vulkan requires that a query must either begin and end inside the same subpass of a render pass instance, or must both begin and end outside of a render pass instance (i.e. contain entire render pass instances). (17.2)");
      }
    }

    --m_CommandListData.m_uiActiveQueriesCounter;

    m_vkCommandBuffer.endQuery(vkQueryPool, uiIndex, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    if (m_CommandListState.m_vkRenderPass != VK_NULL_HANDLE)
    {
      XII_ASSERT_DEV((m_CommandListState.m_uiInsidePassQueries & XII_BIT(queryType)) != 0, "No active inside-pass queries were found.");

      m_CommandListState.m_uiInsidePassQueries &= ~XII_BIT(queryType);
    }
    else
    {
      XII_ASSERT_DEV((m_CommandListState.m_uiOutsidePassQueries & XII_BIT(queryType)) != 0, "No active outside-pass queries were found.");

      m_CommandListState.m_uiOutsidePassQueries &= ~XII_BIT(queryType);
    }
  }
}

void xiiGALCommandListVulkan::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALBufferVulkan*              pBufferVulkan          = xiiDynamicCast<xiiGALBufferVulkan*>(pBuffer);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  // The allocation will stay in the upload heap until the end of the frame at which point all upload pages will be discarded.
  xiiGALStagingBufferAllocationVulkan stagingBufferAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(pBufferVulkan->GetSize());

  void* pMappedMemory = nullptr;
  VK_SUCCEED_OR_RETURN(pVulkanMemoryAllocator->MapMemory(stagingBufferAllocation.m_VulkanAllocation, &pMappedMemory));
  VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, pBufferVulkan->GetSize()));

  pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);
  xiiMemoryUtils::RawByteCopy(pMappedMemory, pSourceData.GetPtr(), pSourceData.GetCount());

  VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(stagingBufferAllocation.m_VulkanAllocation, stagingBufferAllocation.m_uiOffset, pBufferVulkan->GetSize()));
  pVulkanMemoryAllocator->UnmapMemory(stagingBufferAllocation.m_VulkanAllocation);

  UpdateBufferRegion(pBufferVulkan, stagingBufferAllocation.m_vkBuffer, stagingBufferAllocation.m_uiOffset, uiDestinationOffset, pSourceData.GetCount());
}

void xiiGALCommandListVulkan::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pSourceBufferVulkan      = xiiDynamicCast<xiiGALBufferVulkan*>(pSourceBuffer);
  xiiGALBufferVulkan*              pDestinationBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(pDestinationBuffer);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  TransitionOrVerifyBufferState(pSourceBufferVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, vk::AccessFlagBits::eTransferRead, "Using buffer as copy source");
  TransitionOrVerifyBufferState(pDestinationBufferVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Using buffer as copy destination");

  vk::BufferCopy vkBufferCopyRegion = {};
  vkBufferCopyRegion.srcOffset      = 0;
  vkBufferCopyRegion.dstOffset      = 0;
  vkBufferCopyRegion.size           = pSourceBufferVulkan->GetSize();

  FlushBarriers();

  m_vkCommandBuffer.copyBuffer(pSourceBufferVulkan->GetVulkanBuffer(), pDestinationBufferVulkan->GetVulkanBuffer(), 1U, &vkBufferCopyRegion, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan            = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALBufferVulkan*              pSourceBufferVulkan      = xiiDynamicCast<xiiGALBufferVulkan*>(pSourceBuffer);
  xiiGALBufferVulkan*              pDestinationBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(pDestinationBuffer);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  TransitionOrVerifyBufferState(pSourceBufferVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, vk::AccessFlagBits::eTransferRead, "Using buffer as copy source");
  TransitionOrVerifyBufferState(pDestinationBufferVulkan, xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, vk::AccessFlagBits::eTransferWrite, "Using buffer as copy destination");

  vk::BufferCopy vkBufferCopyRegion = {};
  vkBufferCopyRegion.srcOffset      = uiSourceOffset;
  vkBufferCopyRegion.dstOffset      = uiDestinationOffset;
  vkBufferCopyRegion.size           = uiSize;

  FlushBarriers();

  m_vkCommandBuffer.copyBuffer(pSourceBufferVulkan->GetVulkanBuffer(), pDestinationBufferVulkan->GetVulkanBuffer(), 1U, &vkBufferCopyRegion, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

xiiResult xiiGALCommandListVulkan::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALBufferVulkan*              pBufferVulkan          = xiiDynamicCast<xiiGALBufferVulkan*>(pBuffer);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  const xiiGALBufferCreationDescription& bufferDescription = pBufferVulkan->GetDescription();
  MappedBuffer                           mappedBuffer      = {.m_MapType = mapType, .m_DynamicAllocation = {}};
  pMappedData                                              = nullptr;

  if (mapType == xiiGALMapType::Read)
  {
    XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "The buffer must be created with resource usage xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified to be mapped for reading.");

    if (!mapFlags.IsSet(xiiGALMapFlags::DoNotWait))
    {
      xiiLog::Warning("Vulkan backend never waits for GPU when mapping staging buffers for reading. Applications must use fences or other synchronization methods to explicitly synchronize access and use xiiGALMapFlags::DoNotWait flag.");
    }

    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(pBufferVulkan->GetAllocationDescription(), &pMappedData));
    VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(pBufferVulkan->GetAllocationDescription(), 0U, vk::WholeSize));
  }
  else if (mapType == xiiGALMapType::Write)
  {
    if (bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified)
    {
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(pBufferVulkan->GetAllocationDescription(), &pMappedData));
      VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(pBufferVulkan->GetAllocationDescription(), 0U, vk::WholeSize));
    }
    else if (bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic)
    {
      XII_ASSERT_DEV(mapFlags.IsAnySet(xiiGALMapFlags::Discard | xiiGALMapFlags::NoOverWrite), "Failed to map buffer '{}': Vulkan buffer must be mapped for writing with xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite flag.", pBufferVulkan->GetDebugName());

      xiiGALDynamicBufferAllocationVulkan dynamicBufferAllocation = m_CommandListData.m_pDynamicBufferPoolVulkan->Allocate(bufferDescription.m_uiSize);

      void* pMappedMemory = nullptr;
      VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(dynamicBufferAllocation.m_VulkanAllocation, &pMappedMemory));
      VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(dynamicBufferAllocation.m_VulkanAllocation, dynamicBufferAllocation.m_uiOffset, bufferDescription.m_uiSize));

      pMappedData                      = xiiMemoryUtils::AddByteOffset(pMappedMemory, dynamicBufferAllocation.m_uiOffset);
      mappedBuffer.m_DynamicAllocation = dynamicBufferAllocation;
    }
    else
    {
      xiiLog::Error("Only xiiGALResourceUsage::Dynamic, xiiGALResourceUsage::Staging, and xiiGALResourceUsage::Unified Vulkan buffers can be mapped for writing.");
    }
  }
  else if (mapType == xiiGALMapType::ReadWrite)
  {
    xiiLog::Error("xiiGALMapType::ReadWrite is not supported in the Vulkan backend.");
  }
  else
  {
    XII_REPORT_FAILURE("Failed to map Vulkan buffer '{}': unsupported map type {}.", pBufferVulkan->GetDebugName(), xiiArgEnum(mapType));
  }

  if (pMappedData == nullptr)
  {
    xiiLog::Error("Failed to map Vulkan buffer '{}': mapped data pointer is null.", pBufferVulkan->GetDebugName());
    return XII_FAILURE;
  }

  XII_VERIFY(!m_MappedBuffers.Insert(MappedBufferKey{.m_pBufferVulkan = pBufferVulkan, .m_MapType = mapType}, mappedBuffer), "Buffer '{}' has already been mapped.", pBufferVulkan->GetDebugName());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  xiiSharedPtr<xiiGALDeviceVulkan>       pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*              pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALBufferVulkan*                    pBufferVulkan          = xiiDynamicCast<xiiGALBufferVulkan*>(pBuffer);
  const xiiGALBufferCreationDescription& bufferDescription      = pBufferVulkan->GetDescription();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  MappedBufferKey mappedBufferKey = {.m_pBufferVulkan = pBufferVulkan, .m_MapType = mapType};
  MappedBuffer*   pMappedBuffer   = nullptr;

  if (m_MappedBuffers.TryGetValue(MappedBufferKey{.m_pBufferVulkan = pBufferVulkan, .m_MapType = mapType}, pMappedBuffer))
  {
    XII_ASSERT_DEV(pMappedBuffer->m_MapType == mapType, "Map type (expected: {}, actual: {}).", xiiArgEnum(mapType), xiiArgEnum(pMappedBuffer->m_MapType));

    if (mapType == xiiGALMapType::Read)
    {
      if (bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified)
      {
        VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(pBufferVulkan->GetAllocationDescription(), 0U, vk::WholeSize));
        pVulkanMemoryAllocator->UnmapMemory(pBufferVulkan->GetAllocationDescription());
      }
    }
    else if (mapType == xiiGALMapType::Write)
    {
      if (bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified)
      {
        VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(pBufferVulkan->GetAllocationDescription(), 0U, vk::WholeSize));
        pVulkanMemoryAllocator->UnmapMemory(pBufferVulkan->GetAllocationDescription());
      }
      else if (bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic)
      {
        VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(pMappedBuffer->m_DynamicAllocation.m_VulkanAllocation, pMappedBuffer->m_DynamicAllocation.m_uiOffset, bufferDescription.m_uiSize));
        pVulkanMemoryAllocator->UnmapMemory(pMappedBuffer->m_DynamicAllocation.m_VulkanAllocation);

        UpdateBufferRegion(pBufferVulkan, pMappedBuffer->m_DynamicAllocation.m_vkBuffer, pMappedBuffer->m_DynamicAllocation.m_uiOffset, 0U, bufferDescription.m_uiSize);
      }
    }

    XII_VERIFY(m_MappedBuffers.Remove(mappedBufferKey), "");
  }
  else
  {
    xiiLog::Error("Failed to unmap buffer '{}'. The buffer has either been unmapped, or has not been mapped.", pBufferVulkan->GetDebugName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  xiiGALTextureVulkan* pTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(pTexture);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  UpdateTextureRegion(subresourceData.m_pData.GetPtr(), subresourceData.m_uiStride, subresourceData.m_uiDepthStride, pTextureVulkan, textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureBox);
}

void xiiGALCommandListVulkan::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  xiiGALTextureVulkan* pSourceTextureVulkan      = xiiDynamicCast<xiiGALTextureVulkan*>(pSourceTexture);
  xiiGALTextureVulkan* pDestinationTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(pDestinationTexture);

  const xiiGALTextureCreationDescription& sourceTextureDescription      = pSourceTextureVulkan->GetDescription();
  const xiiGALTextureCreationDescription& destinationTextureDescription = pDestinationTextureVulkan->GetDescription();
  xiiGALMipLevelProperties                sourceMipLevelProperties      = xiiGALTextureUtilities::GetMipLevelProperties(sourceTextureDescription, 0);

  if (sourceTextureDescription.m_Usage != xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage != xiiGALResourceUsage::Staging)
  {
    vk::ImageCopy vkImageCopyRegion = {};
    vkImageCopyRegion.extent.width  = sourceMipLevelProperties.m_LogicalSize.width;
    vkImageCopyRegion.extent.height = xiiMath::Max(sourceMipLevelProperties.m_LogicalSize.height, 1U);
    vkImageCopyRegion.extent.depth  = xiiMath::Max(sourceMipLevelProperties.m_uiDepth, 1U);

    auto GetAspectFlags = [](xiiGALResourceFormat::Enum format) -> vk::ImageAspectFlags {
      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);

      switch (formatProperties.m_ComponentType)
      {
        case xiiGALResourceFormatComponentType::Depth:
          return vk::ImageAspectFlagBits::eDepth;
        case xiiGALResourceFormatComponentType::DepthStencil:
          return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
          return vk::ImageAspectFlagBits::eColor;
      }
    };

    vk::ImageAspectFlags vkAspectFlags = GetAspectFlags(sourceTextureDescription.m_Format);
    XII_ASSERT_DEV(vkAspectFlags == GetAspectFlags(destinationTextureDescription.m_Format), "The Vulkan specification requires that the destination and source aspect flags are equivalent.");

    vkImageCopyRegion.srcSubresource.baseArrayLayer = 0;
    vkImageCopyRegion.srcSubresource.layerCount     = 1;
    vkImageCopyRegion.srcSubresource.mipLevel       = 0;
    vkImageCopyRegion.srcSubresource.aspectMask     = vkAspectFlags;
    vkImageCopyRegion.srcOffset.x                   = 0;
    vkImageCopyRegion.srcOffset.y                   = 0;
    vkImageCopyRegion.srcOffset.z                   = 0;

    vkImageCopyRegion.dstSubresource.baseArrayLayer = 0;
    vkImageCopyRegion.dstSubresource.layerCount     = 1;
    vkImageCopyRegion.dstSubresource.mipLevel       = 0;
    vkImageCopyRegion.dstSubresource.aspectMask     = vkAspectFlags;
    vkImageCopyRegion.dstOffset.x                   = 0;
    vkImageCopyRegion.dstOffset.y                   = 0;
    vkImageCopyRegion.dstOffset.z                   = 0;

    CopyTextureRegion(pSourceTextureVulkan, pDestinationTextureVulkan, vkImageCopyRegion);
  }
  else if (sourceTextureDescription.m_Usage == xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage != xiiGALResourceUsage::Staging)
  {
    XII_ASSERT_DEV(sourceTextureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Attempting to copy from staging texture that was not created with the xiiGALCPUAccessFlag::Write flag.");
    XII_ASSERT_DEV(pSourceTextureVulkan->GetResourceState() == xiiGALResourceStateFlags::CopySource, "Source staging texture must permanently be in xiiGALResourceStateFlags::CopySource resources state.");

    // Address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)

    // bufferOffset must be a multiple of 4 (18.4)
    // If the calling command's VkImage parameter is a compressed image, bufferOffset must be a multiple of the compressed texel block size in bytes (18.4).
    // This is automatically guaranteed as MipWidth and MipHeight are rounded to block size.

    const xiiUInt64 uiSourceBufferOffset = xiiGALTextureUtilities::GetStagingTextureLocationOffset(sourceTextureDescription, 0, 0, xiiGALTextureVulkan::s_uiStagingBufferOffsetAlignment, 0, 0, 0);

    xiiBoundingBoxU32 destinationBox = xiiBoundingBoxU32::MakeZero();
    destinationBox.m_vMax.x          = sourceMipLevelProperties.m_LogicalSize.width;
    destinationBox.m_vMax.y          = sourceMipLevelProperties.m_LogicalSize.height;
    destinationBox.m_vMax.z          = sourceMipLevelProperties.m_uiDepth;

    // For storage width, GetStagingTextureLocationOffset assumes texels are tightly packed
    CopyBufferToTexture(pSourceTextureVulkan->GetVulkanStagingBuffer(), uiSourceBufferOffset, sourceMipLevelProperties.m_StorageSize.width, pDestinationTextureVulkan, destinationBox, 0, 0);
  }
  else if (sourceTextureDescription.m_Usage != xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage == xiiGALResourceUsage::Staging)
  {
    XII_ASSERT_DEV(destinationTextureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Attempting to copy from staging texture that was not created with the xiiGALCPUAccessFlag::Read flag.");
    XII_ASSERT_DEV(pDestinationTextureVulkan->GetResourceState() == xiiGALResourceStateFlags::CopyDestination, "Destination staging texture must permanently be in xiiGALResourceStateFlags::CopyDestination resources state.");

    // Address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
    const xiiUInt64 uiDestinationBufferOffset = xiiGALTextureUtilities::GetStagingTextureLocationOffset(destinationTextureDescription, 0, 0, xiiGALTextureVulkan::s_uiStagingBufferOffsetAlignment, 0, 0, 0);

    const xiiGALMipLevelProperties destinationMipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(destinationTextureDescription, 0);

    xiiBoundingBoxU32 sourceBox = xiiBoundingBoxU32::MakeZero();
    sourceBox.m_vMax.x          = sourceMipLevelProperties.m_LogicalSize.width;
    sourceBox.m_vMax.y          = sourceMipLevelProperties.m_LogicalSize.height;
    sourceBox.m_vMax.z          = sourceMipLevelProperties.m_uiDepth;

    // For storage width, GetStagingTextureLocationOffset assumes texels are tightly packed
    CopyTextureToBuffer(pSourceTextureVulkan, sourceBox, 0, 0, pDestinationTextureVulkan->GetVulkanStagingBuffer(), uiDestinationBufferOffset, destinationMipLevelProperties.m_StorageSize.width);
  }
  else
  {
    XII_REPORT_FAILURE("Copying data between staging textures is not supported and is likely not want you really want to do.");
  }
}

void xiiGALCommandListVulkan::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  xiiGALTextureVulkan* pSourceTextureVulkan      = xiiDynamicCast<xiiGALTextureVulkan*>(pSourceTexture);
  xiiGALTextureVulkan* pDestinationTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(pDestinationTexture);

  const xiiGALTextureCreationDescription& sourceTextureDescription      = pSourceTextureVulkan->GetDescription();
  const xiiGALTextureCreationDescription& destinationTextureDescription = pDestinationTextureVulkan->GetDescription();

  if (sourceTextureDescription.m_Usage != xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage != xiiGALResourceUsage::Staging)
  {
    xiiVec3U32 boxExtents = box.GetExtents();

    vk::ImageCopy vkImageCopyRegion = {};
    vkImageCopyRegion.extent.width  = boxExtents.x;
    vkImageCopyRegion.extent.height = xiiMath::Max(boxExtents.y, 1U);
    vkImageCopyRegion.extent.depth  = xiiMath::Max(boxExtents.z, 1U);

    auto GetAspectFlags = [](xiiGALResourceFormat::Enum format) -> vk::ImageAspectFlags {
      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);

      switch (formatProperties.m_ComponentType)
      {
        case xiiGALResourceFormatComponentType::Depth:
          return vk::ImageAspectFlagBits::eDepth;
        case xiiGALResourceFormatComponentType::DepthStencil:
          return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
          return vk::ImageAspectFlagBits::eColor;
      }
    };

    vk::ImageAspectFlags vkAspectFlags = GetAspectFlags(sourceTextureDescription.m_Format);
    XII_ASSERT_DEV(vkAspectFlags == GetAspectFlags(destinationTextureDescription.m_Format), "The Vulkan specification requires that the destination and source aspect flags are equivalent.");

    vkImageCopyRegion.srcSubresource.baseArrayLayer = sourceMipLevelData.m_uiArraySlice;
    vkImageCopyRegion.srcSubresource.layerCount     = 1;
    vkImageCopyRegion.srcSubresource.mipLevel       = sourceMipLevelData.m_uiMipLevel;
    vkImageCopyRegion.srcSubresource.aspectMask     = vkAspectFlags;
    vkImageCopyRegion.srcOffset.x                   = box.m_vMin.x;
    vkImageCopyRegion.srcOffset.y                   = box.m_vMin.y;
    vkImageCopyRegion.srcOffset.z                   = box.m_vMin.z;

    vkImageCopyRegion.dstSubresource.baseArrayLayer = destinationMipLevelData.m_uiArraySlice;
    vkImageCopyRegion.dstSubresource.layerCount     = 1;
    vkImageCopyRegion.dstSubresource.mipLevel       = destinationMipLevelData.m_uiMipLevel;
    vkImageCopyRegion.dstSubresource.aspectMask     = vkAspectFlags;
    vkImageCopyRegion.dstOffset.x                   = box.m_vMax.x;
    vkImageCopyRegion.dstOffset.y                   = box.m_vMax.y;
    vkImageCopyRegion.dstOffset.z                   = box.m_vMax.z;

    CopyTextureRegion(pSourceTextureVulkan, pDestinationTextureVulkan, vkImageCopyRegion);
  }
  else if (sourceTextureDescription.m_Usage == xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage != xiiGALResourceUsage::Staging)
  {
    XII_ASSERT_DEV(sourceTextureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Attempting to copy from staging texture that was not created with the xiiGALCPUAccessFlag::Write flag.");
    XII_ASSERT_DEV(pSourceTextureVulkan->GetResourceState() == xiiGALResourceStateFlags::CopySource, "Source staging texture must permanently be in xiiGALResourceStateFlags::CopySource resources state.");

    // Address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)

    // bufferOffset must be a multiple of 4 (18.4)
    // If the calling command's VkImage parameter is a compressed image, bufferOffset must be a multiple of the compressed texel block size in bytes (18.4).
    // This is automatically guaranteed as MipWidth and MipHeight are rounded to block size.

    const xiiUInt64                uiSourceBufferOffset     = xiiGALTextureUtilities::GetStagingTextureLocationOffset(sourceTextureDescription, sourceMipLevelData.m_uiArraySlice, sourceMipLevelData.m_uiMipLevel, xiiGALTextureVulkan::s_uiStagingBufferOffsetAlignment, box.m_vMin.x, box.m_vMin.y, box.m_vMin.z);
    const xiiGALMipLevelProperties sourceMipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(sourceTextureDescription, sourceMipLevelData.m_uiMipLevel);

    xiiBoundingBoxU32 destinationBox = xiiBoundingBoxU32::MakeFromMinMax(vDestinationPoint, box.GetExtents());

    // For storage width, GetStagingTextureLocationOffset assumes texels are tightly packed
    CopyBufferToTexture(pSourceTextureVulkan->GetVulkanStagingBuffer(), uiSourceBufferOffset, sourceMipLevelProperties.m_StorageSize.width, pDestinationTextureVulkan, destinationBox, destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice);
  }
  else if (sourceTextureDescription.m_Usage != xiiGALResourceUsage::Staging && destinationTextureDescription.m_Usage == xiiGALResourceUsage::Staging)
  {
    XII_ASSERT_DEV(destinationTextureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Attempting to copy from staging texture that was not created with the xiiGALCPUAccessFlag::Read flag.");
    XII_ASSERT_DEV(pDestinationTextureVulkan->GetResourceState() == xiiGALResourceStateFlags::CopyDestination, "Destination staging texture must permanently be in xiiGALResourceStateFlags::CopyDestination resources state.");

    // Address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
    const xiiUInt64                uiDestinationBufferOffset     = xiiGALTextureUtilities::GetStagingTextureLocationOffset(destinationTextureDescription, destinationMipLevelData.m_uiArraySlice, destinationMipLevelData.m_uiMipLevel, xiiGALTextureVulkan::s_uiStagingBufferOffsetAlignment, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z);
    const xiiGALMipLevelProperties destinationMipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(destinationTextureDescription, destinationMipLevelData.m_uiMipLevel);

    // For storage width, GetStagingTextureLocationOffset assumes texels are tightly packed
    CopyTextureToBuffer(pSourceTextureVulkan, box, sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pDestinationTextureVulkan->GetVulkanStagingBuffer(), uiDestinationBufferOffset, destinationMipLevelProperties.m_StorageSize.width);
  }
  else
  {
    XII_REPORT_FAILURE("Copying data between staging textures is not supported and is likely not want you really want to do.");
  }
}

void xiiGALCommandListVulkan::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan>        pDeviceVulkan             = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTextureVulkan*                    pSourceTextureVulkan      = xiiDynamicCast<xiiGALTextureVulkan*>(pSourceTexture);
  xiiGALTextureVulkan*                    pDestinationTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(pDestinationTexture);
  const xiiGALTextureCreationDescription& sourceTextureDescription  = pSourceTextureVulkan->GetDescription();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(sourceTextureDescription.m_Format);

  XII_ASSERT_DEV(sourceTextureDescription.m_Format == pDestinationTextureVulkan->GetDescription().m_Format, "Vulkan requires that source and destination textures of a resolve operation have the same format. (18.6)");
  XII_ASSERT_DEV(formatProperties.m_ComponentType != xiiGALResourceFormatComponentType::Depth && formatProperties.m_ComponentType != xiiGALResourceFormatComponentType::DepthStencil, "Vulkan only permits the resolve operation for colour formats.");
#endif

  // srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.6)
  TransitionOrVerifyTextureState(pSourceTextureVulkan, description.m_SourceTextureTransitionMode, xiiGALResourceStateFlags::ResolveSource, vk::ImageLayout::eTransferSrcOptimal, "Resolving multi-sampled texture (xiiGALCommandList::ResolveTextureSubResource)");

  // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.6)
  TransitionOrVerifyTextureState(pDestinationTextureVulkan, description.m_DestinationTextureTransitionMode, xiiGALResourceStateFlags::ResolveDestination, vk::ImageLayout::eTransferDstOptimal, "Resolving multi-sampled texture (xiiGALCommandList::ResolveTextureSubResource)");

  // The aspectMask member of srcSubresource and dstSubresource must only contain VK_IMAGE_ASPECT_COLOR_BIT (18.6)
  vk::ImageAspectFlags vkImageAspectFlags = vk::ImageAspectFlagBits::eColor;

  vk::ImageResolve vkImageResolveRegion              = {};
  vkImageResolveRegion.srcSubresource.baseArrayLayer = description.m_uiSourceSlice;
  vkImageResolveRegion.srcSubresource.layerCount     = 1;
  vkImageResolveRegion.srcSubresource.mipLevel       = description.m_uiSourceMipLevel;
  vkImageResolveRegion.srcSubresource.aspectMask     = vkImageAspectFlags;

  vkImageResolveRegion.dstSubresource.baseArrayLayer = description.m_uiDestinationSlice;
  vkImageResolveRegion.dstSubresource.layerCount     = 1;
  vkImageResolveRegion.dstSubresource.mipLevel       = description.m_uiDestinationMipLevel;
  vkImageResolveRegion.dstSubresource.aspectMask     = vkImageAspectFlags;

  vkImageResolveRegion.srcOffset = vk::Offset3D{};
  vkImageResolveRegion.dstOffset = vk::Offset3D{};

  const xiiGALMipLevelProperties& sourceMipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(sourceTextureDescription, description.m_uiSourceMipLevel);
  vkImageResolveRegion.extent                              = vk::Extent3D{sourceMipLevelProperties.m_LogicalSize.width, sourceMipLevelProperties.m_LogicalSize.height, sourceMipLevelProperties.m_uiDepth};

  FlushBarriers();

  m_vkCommandBuffer.resolveImage(pSourceTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferSrcOptimal, pDestinationTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, 1U, &vkImageResolveRegion, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan  = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiGALTextureVulkan*             pTextureVulkan = pTextureView->GetTexture().Downcast<xiiGALTextureVulkan>();

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "Mip generation is not permitted while a render pass is active.");

  if (!pTextureVulkan->IsInKnownState())
  {
    xiiLog::Error("Unable to generate mips for texture '{}' because the texture state is unknown.", pTextureVulkan->GetDebugName());
    return;
  }

  const xiiGALTextureCreationDescription&     textureDescription = pTextureVulkan->GetDescription();
  const xiiGALTextureViewCreationDescription& viewDescription    = pTextureView->GetDescription();
  const xiiBitflags<xiiGALResourceStateFlags> originalState      = pTextureVulkan->GetResourceState();
  const vk::ImageLayout                       vkOriginalLayout   = pTextureVulkan->GetVulkanImageLayout();
  const vk::PipelineStageFlags                vkOldPipelineStage = xiiVulkanTypeConversions::GetPipelineStageFlags(originalState);

  XII_ASSERT_DEV(viewDescription.m_uiMipLevelCount > 1, "Number of mip levels in the view must be greater than 1.");
  XII_ASSERT_DEV(originalState != xiiGALResourceStateFlags::Undefined, "Attempting to generate mipmaps for texture '{}' which is in xiiGALResourceStateFlags::Undefined state. This is not expected in Vulkan backend as textures are transitioned to a defined state when created.", pTextureVulkan->GetDebugName());

  vk::ImageSubresourceRange vkImageSubresourceRange = {};

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(viewDescription.m_Format);

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Depth)
  {
    vkImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
  }
  else if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
  {
    // If image has a depth / stencil format with both depth and stencil components, then the aspectMask member of subresourceRange must include both VK_IMAGE_ASPECT_DEPTH_BIT and VK_IMAGE_ASPECT_STENCIL_BIT (6.7.3)

    vkImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
  }
  else
  {
    vkImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  vkImageSubresourceRange.baseArrayLayer = viewDescription.m_uiFirstArrayOrDepthSlice;
  vkImageSubresourceRange.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;
  vkImageSubresourceRange.baseMipLevel   = viewDescription.m_uiMostDetailedMip;
  vkImageSubresourceRange.levelCount     = 1;

  vk::ImageBlit vkImageBlitRegion                 = {};
  vkImageBlitRegion.srcSubresource.baseArrayLayer = viewDescription.m_uiFirstArrayOrDepthSlice;
  vkImageBlitRegion.srcSubresource.layerCount     = viewDescription.m_uiArrayOrDepthSlicesCount;
  vkImageBlitRegion.srcSubresource.aspectMask     = vkImageSubresourceRange.aspectMask;
  vkImageBlitRegion.dstSubresource.baseArrayLayer = vkImageBlitRegion.srcSubresource.baseArrayLayer;
  vkImageBlitRegion.dstSubresource.layerCount     = vkImageBlitRegion.srcSubresource.layerCount;
  vkImageBlitRegion.dstSubresource.aspectMask     = vkImageBlitRegion.srcSubresource.aspectMask;
  vkImageBlitRegion.srcOffsets[0]                 = vk::Offset3D{0, 0, 0};
  vkImageBlitRegion.dstOffsets[0]                 = vk::Offset3D{0, 0, 0};

  vkImageSubresourceRange.baseMipLevel = viewDescription.m_uiMostDetailedMip;
  vkImageSubresourceRange.levelCount   = 1;

  if (originalState != xiiGALResourceStateFlags::CopySource)
  {
    TransitionImageLayout(pTextureVulkan->GetVulkanImage(), vkOriginalLayout, vk::ImageLayout::eTransferSrcOptimal, vkImageSubresourceRange, vkOldPipelineStage, vk::PipelineStageFlagBits::eTransfer);
  }

  for (xiiUInt32 uiMip = viewDescription.m_uiMostDetailedMip + 1; uiMip < viewDescription.m_uiMostDetailedMip + viewDescription.m_uiMipLevelCount; ++uiMip)
  {
    vkImageBlitRegion.srcSubresource.mipLevel = uiMip - 1;
    vkImageBlitRegion.dstSubresource.mipLevel = uiMip;

    vkImageBlitRegion.srcOffsets[1] = vk::Offset3D{static_cast<xiiInt32>(xiiMath::Max(textureDescription.m_Size.width >> (uiMip - 1U), 1U)), static_cast<xiiInt32>(xiiMath::Max(textureDescription.m_Size.height >> (uiMip - 1U), 1U)), 1};
    vkImageBlitRegion.dstOffsets[1] = vk::Offset3D{static_cast<xiiInt32>(xiiMath::Max(textureDescription.m_Size.width >> uiMip, 1U)), static_cast<xiiInt32>(xiiMath::Max(textureDescription.m_Size.height >> uiMip, 1U)), 1};

    if (textureDescription.m_Type == xiiGALResourceDimension::Texture3D)
    {
      vkImageBlitRegion.srcOffsets[1].z = xiiMath::Max(textureDescription.m_uiArraySizeOrDepth >> (uiMip - 1U), 1U);
      vkImageBlitRegion.dstOffsets[1].z = xiiMath::Max(textureDescription.m_uiArraySizeOrDepth >> uiMip, 1U);
    }

    vkImageSubresourceRange.baseMipLevel = uiMip;

    if (vkOriginalLayout != vk::ImageLayout::eTransferDstOptimal)
    {
      TransitionImageLayout(pTextureVulkan->GetVulkanImage(), vkOriginalLayout, vk::ImageLayout::eTransferDstOptimal, vkImageSubresourceRange, vkOldPipelineStage, vk::PipelineStageFlagBits::eTransfer);
    }

    FlushBarriers();

    // For sRGB source formats, nonlinear RGB values are converted to linear representation prior to filtering.
    // In case of sRGB destination format, linear RGB values are converted to nonlinear representation before writing the pixel to the image.
    // Source must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
    // Destination must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
    m_vkCommandBuffer.blitImage(pTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferSrcOptimal, pTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, 1U, &vkImageBlitRegion, vk::Filter::eLinear, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    TransitionImageLayout(pTextureVulkan->GetVulkanImage(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, vkImageSubresourceRange, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer);
  }

  const vk::ImageLayout vkAffectedMipLevelLayout = vk::ImageLayout::eTransferSrcOptimal;

  // All affected mip levels are now in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL state.
  if (vkAffectedMipLevelLayout != vkOriginalLayout)
  {
    bool bIsAllSlices = (textureDescription.m_Type != xiiGALResourceDimension::Texture1DArray && textureDescription.m_Type != xiiGALResourceDimension::Texture2DArray && textureDescription.m_Type != xiiGALResourceDimension::TextureCubeArray) || textureDescription.m_uiArraySizeOrDepth == viewDescription.m_uiArrayOrDepthSlicesCount;
    bool bIsAllMips   = viewDescription.m_uiMipLevelCount == textureDescription.m_uiMipLevels;

    if (bIsAllSlices && bIsAllMips)
    {
      pTextureVulkan->SetVulkanImageLayout(vkAffectedMipLevelLayout);
    }
    else
    {
      XII_ASSERT_DEV(vkOriginalLayout != vk::ImageLayout::eUndefined, "Original layout must not be undefined.");

      vkImageSubresourceRange.baseMipLevel = viewDescription.m_uiMostDetailedMip;
      vkImageSubresourceRange.levelCount   = viewDescription.m_uiMipLevelCount;

      // Transition all affected subresources back to original layout.
      FlushBarriers();

      TransitionImageLayout(pTextureVulkan->GetVulkanImage(), vkAffectedMipLevelLayout, vkOriginalLayout, vkImageSubresourceRange, vk::PipelineStageFlagBits::eTransfer, vkOldPipelineStage);

      XII_ASSERT_DEV(pTextureVulkan->GetVulkanImageLayout() == vkOriginalLayout, "");
    }
  }
}

xiiResult xiiGALCommandListVulkan::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALTextureVulkan*             pTextureVulkan         = xiiDynamicCast<xiiGALTextureVulkan*>(pTexture);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  const xiiGALTextureCreationDescription& textureDescription = pTextureVulkan->GetDescription();
  const xiiGALResourceFormatDescription&  formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

  xiiBoundingBoxU32 fullExtentBox = xiiBoundingBoxU32::MakeZero();
  if (pTextureBox == nullptr)
  {
    xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(textureDescription, textureMipLevelData.m_uiMipLevel);

    fullExtentBox.m_vMax.x = mipLevelProperties.m_LogicalSize.width;
    fullExtentBox.m_vMax.y = mipLevelProperties.m_LogicalSize.height;
    fullExtentBox.m_vMax.z = mipLevelProperties.m_uiDepth;

    pTextureBox = &fullExtentBox;
  }

  if (textureDescription.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    if (mapType != xiiGALMapType::Write)
    {
      xiiLog::Error("In Vulkan implementation, dynamic textures can only be mapped for writing.");

      mappedData = xiiGALMappedTextureSubresource();

      return XII_FAILURE;
    }

    if (mapFlags.IsAnySet(xiiGALMapFlags::Discard | xiiGALMapFlags::NoOverWrite))
    {
      xiiLog::Info("In Vulkan implementation, mapping textures with flags xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite has no effect.");
    }

    const vk::PhysicalDeviceLimits&            vkDeviceLimits  = pDeviceVulkan->GetVulkanPhysicalDeviceProperties().limits;
    const xiiGALBufferToTextureCopyDescription copyDescription = xiiGALTextureUtilities::GetBufferToTextureCopyDescription(textureDescription.m_Format, *pTextureBox, static_cast<xiiUInt32>(vkDeviceLimits.optimalBufferCopyRowPitchAlignment));

    xiiGALDynamicBufferAllocationVulkan dynamicBufferAllocation = m_CommandListData.m_pDynamicBufferPoolVulkan->Allocate(copyDescription.m_uiMemorySize);

    void* pMappedMemory = nullptr;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(dynamicBufferAllocation.m_VulkanAllocation, &pMappedMemory));
    VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(dynamicBufferAllocation.m_VulkanAllocation, dynamicBufferAllocation.m_uiOffset, copyDescription.m_uiMemorySize));

    pMappedMemory = xiiMemoryUtils::AddByteOffset(pMappedMemory, dynamicBufferAllocation.m_uiOffset);

    mappedData.m_pData         = pMappedMemory;
    mappedData.m_uiStride      = copyDescription.m_uiRowStride;
    mappedData.m_uiDepthStride = copyDescription.m_uiDepthStride;

    XII_VERIFY(!m_MappedTextures.Insert(MappedTextureKey{.m_pTextureVulkan = pTextureVulkan, .m_uiMipLevel = textureMipLevelData.m_uiMipLevel, .m_uiArraySlice = textureMipLevelData.m_uiArraySlice}, MappedTexture{.m_CopyDescription = copyDescription, .m_DynamicAllocation = dynamicBufferAllocation}), "Mip level {}, slice {}, of texture '{}' has already been mapped.", textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, pTextureVulkan->GetDebugName());
  }
  else if (textureDescription.m_Usage == xiiGALResourceUsage::Staging)
  {
    xiiUInt64                uiSubResourceOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(textureDescription, textureMipLevelData.m_uiArraySlice, textureMipLevelData.m_uiMipLevel, xiiGALTextureVulkan::s_uiStagingBufferOffsetAlignment);
    xiiGALMipLevelProperties mipLevelProperties  = xiiGALTextureUtilities::GetMipLevelProperties(textureDescription, textureMipLevelData.m_uiMipLevel);

    // Address of (x,y,z) = region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
    // For compressed-block formats, RowSize is the size of one compressed row.
    // For non-compressed formats, BlockHeight is 1.
    // For non-compressed formats, BlockWidth is 1.
    xiiUInt64 uiMapStartOffset = uiSubResourceOffset + (pTextureBox->m_vMin.z * mipLevelProperties.m_StorageSize.height + pTextureBox->m_vMin.y) / formatProperties.m_uiBlockHeight * mipLevelProperties.m_uiRowSize + pTextureBox->m_vMin.x / formatProperties.m_uiBlockWidth * xiiUInt64{formatProperties.GetElementSize()};

    void* pMappedMemory = nullptr;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(pVulkanMemoryAllocator->MapMemory(pTextureVulkan->GetStagingBufferAllocationDescription(), &pMappedMemory));

    mappedData.m_pData         = xiiMemoryUtils::AddByteOffset(pMappedMemory, uiMapStartOffset);
    mappedData.m_uiStride      = mipLevelProperties.m_uiRowSize;
    mappedData.m_uiDepthStride = mipLevelProperties.m_uiDepthSliceSize;

    XII_VERIFY(!m_MappedTextures.Insert(MappedTextureKey{.m_pTextureVulkan = pTextureVulkan, .m_uiMipLevel = textureMipLevelData.m_uiMipLevel, .m_uiArraySlice = textureMipLevelData.m_uiArraySlice}, MappedTexture{.m_CopyDescription = {}}), "Mip level {}, slice {}, of texture '{}' has already been mapped.", textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, pTextureVulkan->GetDebugName());

    if (mapType == xiiGALMapType::Read)
    {
      if (!mapFlags.IsSet(xiiGALMapFlags::DoNotWait))
      {
        xiiLog::Warning("Vulkan backend never waits for GPU when mapping staging textures for reading. Applications must use fences or other synchronization methods to explicitly synchronize access and use xiiGALMapFlags::DoNotWait flag.");
      }

      XII_ASSERT_DEV(textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Texture '{}' was not created with xiiGALCPUAccessFlag::Read and cannot be mapped for reading.");

      // Readback memory is not created with HOST_COHERENT flag, so we have to explicitly invalidate the mapped range to make device writes visible to CPU reads.
      XII_ASSERT_DEV(pTextureBox->m_vMax.z >= 1 && pTextureBox->m_vMax.y >= 1, "");
      xiiUInt32 uiBlockAlignedMaxX = xiiMemoryUtils::AlignSize(pTextureBox->m_vMax.x, xiiUInt32{formatProperties.m_uiBlockWidth});
      xiiUInt32 uiBlockAlignedMaxY = xiiMemoryUtils::AlignSize(pTextureBox->m_vMax.y, xiiUInt32{formatProperties.m_uiBlockHeight});
      xiiUInt64 uiMapEndOffset     = ((pTextureBox->m_vMax.z - 1) * mipLevelProperties.m_StorageSize.height + (uiBlockAlignedMaxY - formatProperties.m_uiBlockHeight)) / formatProperties.m_uiBlockHeight * mipLevelProperties.m_uiRowSize + (uiBlockAlignedMaxX / formatProperties.m_uiBlockWidth) * xiiUInt64{formatProperties.GetElementSize()};

      VK_ASSERT_DEV(pVulkanMemoryAllocator->InvalidateAllocation(pTextureVulkan->GetStagingBufferAllocationDescription(), uiMapStartOffset, uiMapEndOffset - uiMapStartOffset));
    }
    else if (mapType == xiiGALMapType::Write)
    {
      XII_ASSERT_DEV(textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Texture '{}' was not created with xiiGALCPUAccessFlag::Write flag and cannot be mapped for writing", pTextureVulkan->GetDebugName());

      // Nothing else to do.
    }
  }
  else
  {
    XII_REPORT_FAILURE("Texture with usage {} cannot currently be mapped in the Vulkan implementation.", textureDescription.m_Usage);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan          = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiVulkanMemoryAllocator*        pVulkanMemoryAllocator = pDeviceVulkan->GetVulkanMemoryAllocator();
  xiiGALTextureVulkan*             pTextureVulkan         = xiiDynamicCast<xiiGALTextureVulkan*>(pTexture);

  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  const xiiGALTextureCreationDescription& textureDescription = pTextureVulkan->GetDescription();

  MappedTextureKey mappedTextureKey = {.m_pTextureVulkan = pTextureVulkan, .m_uiMipLevel = textureMipLevelData.m_uiMipLevel, .m_uiArraySlice = textureMipLevelData.m_uiArraySlice};
  MappedTexture*   mappedTexture    = nullptr;

  if (m_MappedTextures.TryGetValue(mappedTextureKey, mappedTexture))
  {
    VK_ASSERT_DEV(pVulkanMemoryAllocator->FlushAllocation(pTextureVulkan->GetStagingBufferAllocationDescription(), 0U, vk::WholeSize));
    pVulkanMemoryAllocator->UnmapMemory(pTextureVulkan->GetStagingBufferAllocationDescription());

    if (textureDescription.m_Usage == xiiGALResourceUsage::Dynamic)
    {
      CopyBufferToTexture(mappedTexture->m_DynamicAllocation.m_vkBuffer, mappedTexture->m_DynamicAllocation.m_uiOffset, mappedTexture->m_CopyDescription.m_uiRowStrideInTexels, pTextureVulkan, mappedTexture->m_CopyDescription.m_Region, textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice);
    }
    else if (textureDescription.m_Usage == xiiGALResourceUsage::Staging)
    {
      if (textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
      {
        // Nothing needs to be done.
      }
      else if (textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
      {
        // Nothing needs to be done.
      }
    }
    else
    {
      XII_REPORT_FAILURE("Texture with usage {} cannot currently be mapped in the Vulkan implementation.", textureDescription.m_Usage);
    }

    XII_VERIFY(m_MappedTextures.Remove(mappedTextureKey), "");
  }
  else
  {
    xiiLog::Error("Failed to unmap mip level {}, slice {}, of texture {}. The texture has either been unmapped, or has not been mapped.", textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, pTextureVulkan->GetDebugName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::SetShadingRatePlatform(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan     = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const auto&                      extensionFeatures = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

  if (extensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
  {
    vk::FragmentShadingRateCombinerOpKHR primitiveCombinerOps[2] = {xiiVulkanTypeConversions::GetFragmentShadingRateCombinerOp(primitiveCombinerFlags), xiiVulkanTypeConversions::GetFragmentShadingRateCombinerOp(textureCombinerFlags)};
    vk::Extent2D                         vkFragmentSize          = xiiVulkanTypeConversions::ShadingRateToFragmentSize(baseRateFlags);

    m_vkCommandBuffer.setFragmentShadingRateKHR(&vkFragmentSize, primitiveCombinerOps, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_CommandListState.m_bIsShadingRateSet = true;
  }
  else if (extensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False)
  {
    // Ignored.
    XII_ASSERT_DEV(baseRateFlags == xiiGALShadingRateFlags::_1X1, "Fragment density map is supported, but fragment shading rate attachment is not. Setting a non-default shading rate is not supported in this configuration.");
    XII_ASSERT_DEV(primitiveCombinerFlags == xiiGALShadingRateCombinerFlags::PassThrough, "Fragment density map is supported, but fragment shading rate attachment is not. Setting a non-default primitive combiner is not supported in this configuration.");
    XII_ASSERT_DEV(textureCombinerFlags == xiiGALShadingRateCombinerFlags::CombinerOverride, "Fragment density map is supported, but fragment shading rate attachment is not. Setting a non-default texture combiner is not supported in this configuration.");
  }
  else
  {
    xiiLog::Error("Attempting to set shading rate on a device that does not support fragment shading rate attachment.");
  }
}

void xiiGALCommandListVulkan::TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

  for (xiiUInt32 uiBarrierIndex = 0; uiBarrierIndex < pResourceBarriers.GetCount(); ++uiBarrierIndex)
  {
    const xiiGALStateTransitionDescription& barrier = pResourceBarriers[uiBarrierIndex];

    if (barrier.m_TransitionType == xiiGALStateTransitionType::Begin)
    {
      // Skip begin split-barriers.
      XII_ASSERT_DEV(!barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState), "Resource state can not be updated in a begin-split barrier.");
      continue;
    }
    if (barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::Aliasing))
    {
      auto GetResourceBindFlags = [](xiiGALResource* pResource) -> xiiBitflags<xiiGALBindFlags> {
        if (xiiGALTexture* pTexture = xiiDynamicCast<xiiGALTexture*>(pResource))
        {
          return pTexture->GetDescription().m_BindFlags;
        }
        else if (xiiGALBuffer* pBuffer = xiiDynamicCast<xiiGALBuffer*>(pResource))
        {
          return pBuffer->GetDescription().m_BindFlags;
        }
        else
        {
          return xiiGALBindFlags::BindAll;
        }
      };

      vk::PipelineStageFlags vkSourcePipelineStageFlags = static_cast<vk::PipelineStageFlagBits>(0);
      vk::AccessFlags        vkSourceAccessFlags        = vk::AccessFlagBits::eNone;
      xiiVulkanTypeConversions::GetPermittedStagesAndAccessFlags(GetResourceBindFlags(barrier.m_pPreviousResource), vkSourcePipelineStageFlags, vkSourceAccessFlags);

      vk::PipelineStageFlags vkDestinationPipelineStageFlags = static_cast<vk::PipelineStageFlagBits>(0);
      vk::AccessFlags        vkDestinationAccessFlags        = vk::AccessFlagBits::eNone;
      xiiVulkanTypeConversions::GetPermittedStagesAndAccessFlags(GetResourceBindFlags(barrier.m_pResource), vkDestinationPipelineStageFlags, vkDestinationAccessFlags);

      MemoryBarrier(vkSourceAccessFlags, vkDestinationAccessFlags, vkSourcePipelineStageFlags, vkDestinationPipelineStageFlags);
    }
    else
    {
      XII_ASSERT_DEV(barrier.m_TransitionType == xiiGALStateTransitionType::Immediate || barrier.m_TransitionType == xiiGALStateTransitionType::End, "Unexpected barrier type.");

      if (xiiGALTextureVulkan* pTextureVulkan = xiiDynamicCast<xiiGALTextureVulkan*>(barrier.m_pResource))
      {
        vk::ImageSubresourceRange vkImageSubresourceRange = {};
        vkImageSubresourceRange.aspectMask                = vk::ImageAspectFlagBits::eNone;
        vkImageSubresourceRange.baseMipLevel              = barrier.m_uiFirstMipLevel;
        vkImageSubresourceRange.levelCount                = (barrier.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : barrier.m_uiMipLevelCount;
        vkImageSubresourceRange.baseArrayLayer            = barrier.m_uiFirstArraySlice;
        vkImageSubresourceRange.layerCount                = (barrier.m_uiArraySliceCount == XII_GAL_REMAINING_ARRAY_SLICES) ? VK_REMAINING_ARRAY_LAYERS : barrier.m_uiArraySliceCount;

        TransitionTextureState(pTextureVulkan, barrier.m_OldState, barrier.m_NewState, barrier.m_TransitionFlags, &vkImageSubresourceRange);
      }
      else if (xiiGALBufferVulkan* pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(barrier.m_pResource))
      {
        TransitionBufferState(pBufferVulkan, barrier.m_OldState, barrier.m_NewState, barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState));
      }
      else if (xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = xiiDynamicCast<xiiGALBottomLevelASVulkan*>(barrier.m_pResource))
      {
        xiiBitflags<xiiGALResourceStateFlags> oldState = barrier.m_OldState;
        if (oldState == xiiGALResourceStateFlags::Unknown)
        {
          oldState = pBottomLevelASVulkan->GetResourceState();
        }

        if (oldState == xiiGALResourceStateFlags::Unknown)
        {
          xiiLog::Error("Failed to transition BLAS '{}' because old state is unknown.", pBottomLevelASVulkan->GetDebugName());
          continue;
        }

        MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(barrier.m_NewState), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(barrier.m_NewState));

        if (barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
        {
          pBottomLevelASVulkan->SetResourceState(barrier.m_NewState);
        }
      }
      else if (xiiGALTopLevelASVulkan* pTopLevelASVulkan = xiiDynamicCast<xiiGALTopLevelASVulkan*>(barrier.m_pResource))
      {
        xiiBitflags<xiiGALResourceStateFlags> oldState = barrier.m_OldState;
        if (oldState == xiiGALResourceStateFlags::Unknown)
        {
          oldState = pTopLevelASVulkan->GetResourceState();
        }

        if (oldState == xiiGALResourceStateFlags::Unknown)
        {
          xiiLog::Error("Failed to transition TLAS '{}' because old state is unknown.", pTopLevelASVulkan->GetDebugName());
          continue;
        }

        MemoryBarrier(xiiVulkanTypeConversions::GetAccessFlags(oldState), xiiVulkanTypeConversions::GetAccessFlags(barrier.m_NewState), xiiVulkanTypeConversions::GetPipelineStageFlags(oldState), xiiVulkanTypeConversions::GetPipelineStageFlags(barrier.m_NewState));

        if (barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
        {
          pTopLevelASVulkan->SetResourceState(barrier.m_NewState);
        }
      }
      else
      {
        XII_REPORT_FAILURE("Unsupported resource type.");
      }
    }
  }
}

void xiiGALCommandListVulkan::EnqueueSignalPlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  xiiGALFenceVulkan* pFenceVulkan = xiiDynamicCast<xiiGALFenceVulkan*>(pFence);
  FenceInfo          fenceInfo    = {.m_pFenceVulkan = pFenceVulkan, .m_uiWaitValue = uiValue};

  m_SignalFences.PushBack(fenceInfo);
}

void xiiGALCommandListVulkan::DeviceWaitForFencePlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  xiiGALFenceVulkan* pFenceVulkan = xiiDynamicCast<xiiGALFenceVulkan*>(pFence);
  FenceInfo          fenceInfo    = {.m_pFenceVulkan = pFenceVulkan, .m_uiWaitValue = uiValue};

  m_WaitFences.PushBack(fenceInfo);
}

void xiiGALCommandListVulkan::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (pDeviceVulkan->GetDebugMode() == xiiGALDeviceVulkan::DebugMode::Utils)
  {
    xiiStringBuilder tmp;

    vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
    vkDebugUtilsLabel.pNext                  = nullptr;
    vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
    vkDebugUtilsLabel.color[0]               = color.r;
    vkDebugUtilsLabel.color[1]               = color.g;
    vkDebugUtilsLabel.color[2]               = color.b;
    vkDebugUtilsLabel.color[3]               = color.a;

    m_vkCommandBuffer.beginDebugUtilsLabelEXT(&vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::EndDebugGroupPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (pDeviceVulkan->GetDebugMode() == xiiGALDeviceVulkan::DebugMode::Utils)
  {
    m_vkCommandBuffer.endDebugUtilsLabelEXT(pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (pDeviceVulkan->GetDebugMode() == xiiGALDeviceVulkan::DebugMode::Utils)
  {
    xiiStringBuilder tmp;

    vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
    vkDebugUtilsLabel.pNext                  = nullptr;
    vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
    vkDebugUtilsLabel.color[0]               = color.r;
    vkDebugUtilsLabel.color[1]               = color.g;
    vkDebugUtilsLabel.color[2]               = color.b;
    vkDebugUtilsLabel.color[3]               = color.a;

    m_vkCommandBuffer.insertDebugUtilsLabelEXT(&vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
}

void xiiGALCommandListVulkan::InvalidateStatePlatform()
{
  m_CommandListFlags = {};
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

  m_CommandListData.Invalidate();

  XII_ASSERT_DEV(m_MappedBuffers.IsEmpty(), "There are outstanding buffers that have not been unmapped.");
  XII_ASSERT_DEV(m_MappedTextures.IsEmpty(), "There are outstanding textures that have not been unmapped.");
}

void xiiGALCommandListVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandBuffer, sName.GetData(tmp));
}

void xiiGALCommandListVulkan::PrepareForDraw()
{
  XII_ASSERT_DEBUG(m_vkCommandBuffer != VK_NULL_HANDLE, "Invalid command buffer.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiUInt32 uiSlot = 0; uiSlot < m_VertexStreams.GetCount(); ++uiSlot)
  {
    if (xiiGALBuffer* pBuffer = m_VertexStreams[uiSlot].m_pBuffer)
    {
      VerifyBufferState(pBuffer, xiiGALResourceStateFlags::VertexBuffer, "Using vertex buffers");
    }
  }
#endif

  if (m_CommandListFlags.IsSet(CommandListFlags::CommittedVertexBuffersModified))
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

    xiiTemporaryHybridArray<vk::Buffer, 2U> vkVertexBuffers;
    vkVertexBuffers.SetCountUninitialized(m_VertexStreams.GetCount());

    xiiTemporaryHybridArray<vk::DeviceSize, 2U> vkVertexBufferOffsets;
    vkVertexBufferOffsets.SetCountUninitialized(m_VertexStreams.GetCount());

    for (xiiUInt32 uiSlot = 0; uiSlot < m_VertexStreams.GetCount(); ++uiSlot)
    {
      VertexStreamDescription& vertexStream = m_VertexStreams[uiSlot];

      if (xiiGALBufferVulkan* pVertexBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(vertexStream.m_pBuffer))
      {
        vkVertexBuffers[uiSlot]       = pVertexBufferVulkan->GetVulkanBuffer();
        vkVertexBufferOffsets[uiSlot] = vertexStream.m_uiOffset;
      }
      else
      {
        // We cannot bind a null vertex buffer in Vulkan, so we use a dedicated null vertex buffer.
        vkVertexBuffers[uiSlot]       = m_CommandListData.m_pNullVertexBuffer->GetVulkanBuffer();
        vkVertexBufferOffsets[uiSlot] = 0U;
      }
    }

    if (!m_VertexStreams.IsEmpty())
    {
      m_vkCommandBuffer.bindVertexBuffers(0, m_VertexStreams.GetCount(), vkVertexBuffers.GetData(), vkVertexBufferOffsets.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }

    m_CommandListFlags.Remove(CommandListFlags::CommittedVertexBuffersModified);
  }
}

void xiiGALCommandListVulkan::PrepareForIndexedDraw(xiiEnum<xiiGALValueType> indexType)
{
  PrepareForDraw();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  VerifyBufferState(m_pIndexBuffer, xiiGALResourceStateFlags::IndexBuffer, "Indexed draw call");

  XII_ASSERT_DEV(indexType == xiiGALValueType::UInt16 || indexType == xiiGALValueType::UInt32, "Unsupported index type, only xiiGALValueType::UInt16 or xiiGALValueType::UInt32 are supported.");
#endif

  if (m_CommandListFlags.IsSet(CommandListFlags::CommittedIndexBufferModified))
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    xiiGALBufferVulkan*              pBufferVulkan = xiiDynamicCast<xiiGALBufferVulkan*>(m_pIndexBuffer);

    vk::IndexType vkIndexType = xiiVulkanTypeConversions::GetIndexType(indexType);

    m_vkCommandBuffer.bindIndexBuffer(pBufferVulkan->GetVulkanBuffer(), m_uiIndexDataOffset, vkIndexType, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    m_CommandListState.m_vkIndexBuffer       = pBufferVulkan->GetVulkanBuffer();
    m_CommandListState.m_vkIndexBufferOffset = m_uiIndexDataOffset;
    m_CommandListState.m_vkIndexType         = vkIndexType;

    m_CommandListFlags.Remove(CommandListFlags::CommittedIndexBufferModified);
  }
}

void xiiGALCommandListVulkan::PrepareForDispatchCompute()
{
}

void xiiGALCommandListVulkan::PrepareForRayTracing()
{
}

[[nodiscard]] inline bool ResourceStateHasWriteAccess(xiiBitflags<xiiGALResourceStateFlags> flags)
{
  xiiBitflags<xiiGALResourceStateFlags> writeAccessStates = xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::BuildASWrite;

  return writeAccessStates.IsAnySet(flags);
}

void xiiGALCommandListVulkan::TransitionBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, const bool bUpdateBufferState)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

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
      xiiLog::Error("The state ({}) of buffer '{}' does not match the old state ({}) specified by the barrier.", xiiArgEnum(pBufferVulkan->GetResourceState()), pBufferVulkan->GetDebugName(), xiiArgEnum(oldState));
    }
  }

  // Always add barrier after writes.
  const bool bAfterWrite = ResourceStateHasWriteAccess(oldState);

  if (((oldState & newState) != newState) || bAfterWrite)
  {
    vk::AccessFlags        vkOldAccessFlags = xiiVulkanTypeConversions::GetAccessFlags(oldState);
    vk::AccessFlags        vkNewAccessFlags = xiiVulkanTypeConversions::GetAccessFlags(newState);
    vk::PipelineStageFlags vkOldStages      = xiiVulkanTypeConversions::GetPipelineStageFlags(oldState);
    vk::PipelineStageFlags vkNewStages      = xiiVulkanTypeConversions::GetPipelineStageFlags(newState);

    MemoryBarrier(vkOldAccessFlags, vkNewAccessFlags, vkOldStages, vkNewStages);

    if (bUpdateBufferState)
    {
      pBufferVulkan->SetResourceState(newState);
    }
  }
}

void xiiGALCommandListVulkan::BufferMemoryBarrier(xiiGALBufferVulkan* pBufferVulkan, vk::AccessFlags newAccessFlags)
{
  XII_ASSERT_DEV(pBufferVulkan != nullptr, "The buffer has been invalidated.");

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

void xiiGALCommandListVulkan::TransitionTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, xiiBitflags<xiiGALStateTransitionFlags> flags, vk::ImageSubresourceRange* pSubresourceRange /*= nullptr*/)
{
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

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
      xiiLog::Error("The state ({}) of texture '{}' does not match the old state ({}) specified by the barrier.", xiiArgEnum(pTextureVulkan->GetResourceState()), pTextureVulkan->GetDebugName(), xiiArgEnum(oldState));
    }
  }

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
    const xiiGALTextureCreationDescription& textureDescription = pTextureVulkan->GetDescription();
    const xiiGALResourceFormatDescription&  formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

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

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan       = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const auto&                      extensionFeatures   = pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();
  const bool                       bFragmentDensityMap = extensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False;
  const vk::ImageLayout            oldLayout           = flags.IsSet(xiiGALStateTransitionFlags::DiscardContent) ? vk::ImageLayout::eUndefined : xiiVulkanTypeConversions::GetImageLayout(oldState, false, bFragmentDensityMap);
  const vk::ImageLayout            newLayout           = xiiVulkanTypeConversions::GetImageLayout(newState, false, bFragmentDensityMap);
  const vk::PipelineStageFlags     oldStages           = xiiVulkanTypeConversions::GetPipelineStageFlags(oldState);
  const vk::PipelineStageFlags     newStages           = xiiVulkanTypeConversions::GetPipelineStageFlags(newState);

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
  XII_ASSERT_DEV(pTextureVulkan != nullptr, "The texture has beeen invalidated.");
  XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

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

void xiiGALCommandListVulkan::TransitionOrVerifyBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiEnum<xiiGALStateTransitionMode> transitionMode, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::AccessFlagBits expectedAccessFlags, const char* szOperationName)
{
  if (transitionMode == xiiGALStateTransitionMode::Transition)
  {
    XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

    if (pBufferVulkan->IsInKnownState())
    {
      TransitionBufferState(pBufferVulkan, xiiGALResourceStateFlags::Unknown, requiredState, true);

      XII_ASSERT_DEV(pBufferVulkan->CheckAccessFlags(expectedAccessFlags), "");
    }
  }
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  else if (transitionMode == xiiGALStateTransitionMode::Verify)
  {
    if (pBufferVulkan->IsInKnownState() && !pBufferVulkan->CheckState(requiredState))
    {
      xiiLog::Error("{} requires buffer '{}' to be transitioned to {} state. Actual buffer state: {}. Use appropriate transition flags or explicitly transition the buffer using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pBufferVulkan->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pBufferVulkan->GetResourceState()));
    }
  }
#else
  XII_IGNORE_UNUSED(szOperationName);
  XII_IGNORE_UNUSED(expectedAccessFlags);
#endif
}

void xiiGALCommandListVulkan::TransitionOrVerifyTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiEnum<xiiGALStateTransitionMode> transitionMode, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::ImageLayout expectedLayout, const char* szOperationName)
{
  if (transitionMode == xiiGALStateTransitionMode::Transition)
  {
    XII_ASSERT_DEV(m_CommandListState.m_vkRenderPass == VK_NULL_HANDLE, "State transitions are not permitted while a render pass is active.");

    if (pTextureVulkan->IsInKnownState())
    {
      TransitionTextureState(pTextureVulkan, xiiGALResourceStateFlags::Unknown, requiredState, xiiGALStateTransitionFlags::UpdateState);

      XII_ASSERT_DEV(pTextureVulkan->GetVulkanImageLayout() == expectedLayout, "");
    }
  }
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  else if (transitionMode == xiiGALStateTransitionMode::Verify)
  {
    if (pTextureVulkan->IsInKnownState() && !pTextureVulkan->CheckState(requiredState))
    {
      xiiLog::Error("{} requires texture '{}' to be transitioned to {} state. Actual texture state: {}. Use appropriate transition flags or explicitly transition the texture using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pTextureVulkan->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pTextureVulkan->GetResourceState()));
    }
  }
#else
  XII_IGNORE_UNUSED(szOperationName);
  XII_IGNORE_UNUSED(expectedLayout);
#endif
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandListVulkan);
