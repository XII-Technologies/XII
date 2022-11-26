#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Containers/Bitfield.h>

#include <vulkan/vulkan.hpp>

class xiiGALBufferVulkan;
class xiiGALTextureVulkan;
class xiiGALRenderTargetViewVulkan;
class xiiGALResourceViewVulkan;
class xiiGALUnorderedAccessViewVulkan;

/// \brief
class XII_RENDERERVULKAN_DLL xiiPipelineBarrierVulkan
{
public:
  /// \name Barrier handling
  ///@{

  /// \brief Set the active command buffer that barriers are flushed into.
  void SetCommandBuffer(vk::CommandBuffer* pCommandBuffer);

  /// \brief Flush accumulated changes as a barrier into the active command buffer.
  void Flush();

  /// \brief Should be called when the active command buffer is submitted. This clear various states as a submit works like a full barrier.
  void Submit();

  /// \brief Full barrier for debugging purposes.
  void FullBarrier();

  bool IsDirty() const;

  ///@}
  /// \name Buffer handling
  ///@{

  /// \brief Need to be called to remove the buffer from tracking.
  void BufferDestroyed(const xiiGALBufferVulkan* pBuffer);

  void AccessBuffer(const xiiGALBufferVulkan* pBuffer, vk::DeviceSize offset, vk::DeviceSize length, vk::PipelineStageFlags srcStages, vk::AccessFlags srcAccess, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess);
  bool IsDirty(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize length, vk::AccessFlags dstAccess);
  ///@}
  /// \name Image handling
  ///@{

  /// \brief Force, sets the layout of the texture's image. Most images are not created in their preferred layout on creation.
  /// \param pTexture The texture whose image the initial state should be set on.
  /// \param dstLayout The layout that the image currently is in.
  /// \param dstStages In what stage the image was changed into the current layout.
  /// \param dstAccess The access operation that changed the image into the current layout during dstStages.
  void SetInitialImageState(const xiiGALTextureVulkan* pTexture, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages = vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlags dstAccess = {});

  /// \brief Need to be called to remove the texture's image from tracking.
  void TextureDestroyed(const xiiGALTextureVulkan* pTexture);

  /// \brief Transition the given texture's image into the given layout to be used in the given stage.
  /// \param pTexture The texture whose image's layout is to be changed.
  /// \param dstLayout The layout that the image is to be changed into.
  /// \param dstStages In what stages the image needs to be in the new layout.
  /// \param dstAccess What access we will be doing on the image in dstStages.
  /// \param bDiscardSource Discard the previous layout, replaces current layout with unknown.
  void EnsureImageLayout(const xiiGALTextureVulkan* pTexture, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource = false);
  void EnsureImageLayout(const xiiGALRenderTargetViewVulkan* pTextureView, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource = false);
  void EnsureImageLayout(const xiiGALResourceViewVulkan* pTextureView, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource = false);
  void EnsureImageLayout(const xiiGALUnorderedAccessViewVulkan* pTextureView, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource = false);
  void EnsureImageLayout(const xiiGALTextureVulkan* pTexture, vk::ImageSubresourceRange subResources, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource = false);

  bool IsDirty(vk::Image image, const vk::ImageSubresourceRange& subResources) const;
  ///@}

private:
  struct SubElementState
  {
    vk::PipelineStageFlags m_stages;
    vk::AccessFlags        m_accessMask;
    vk::ImageLayout        m_layout;

    bool operator==(const SubElementState& rhs) const
    {
      return m_stages == rhs.m_stages && m_accessMask == rhs.m_accessMask && m_layout == rhs.m_layout;
    }
  };

  struct ImageState
  {
    const xiiGALTextureVulkan*                m_pTexture = nullptr;
    xiiBitfield<xiiHybridArray<xiiUInt32, 1>> m_dirty;
    xiiHybridArray<SubElementState, 1>        m_subElementLayout;
  };

  struct SubBufferState
  {
    vk::DeviceSize         m_offset;
    vk::DeviceSize         m_length;
    vk::PipelineStageFlags m_stages;
    vk::AccessFlags        m_accessMask;
  };

  struct BufferState
  {
    const xiiGALBufferVulkan*                 m_pBuffer = nullptr;
    xiiBitfield<xiiHybridArray<xiiUInt32, 1>> m_dirty;
    xiiHybridArray<SubBufferState, 1>         m_subBufferState;
  };

  bool AddBufferBarrierInternal(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize length, vk::PipelineStageFlags srcStages, vk::AccessFlags srcAccess, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess);
  bool IsDirtyInternal(const BufferState& state, const SubBufferState& subState) const;

  bool AddImageBarrierInternal(vk::Image image, const vk::ImageSubresourceRange& subResources, vk::ImageLayout srcLayout, vk::AccessFlags srcAccess, vk::ImageLayout dstLayout, vk::AccessFlags dstAccess, bool bDiscardSource);
  bool IsDirtyInternal(const ImageState& state, const vk::ImageSubresourceRange& subResources) const;

private:
  static constexpr vk::AccessFlags s_readAccess = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
    vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eUniformRead |
    vk::AccessFlagBits::eInputAttachmentRead | vk::AccessFlagBits::eShaderRead |
    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentRead |
    vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eHostRead |
    vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eTransformFeedbackCounterReadEXT |
    vk::AccessFlagBits::eConditionalRenderingReadEXT | vk::AccessFlagBits::eColorAttachmentReadNoncoherentEXT |
    vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eFragmentDensityMapReadEXT |
    vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR | vk::AccessFlagBits::eCommandPreprocessReadNV |
    vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eFragmentDensityMapReadEXT |
    vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR | vk::AccessFlagBits::eCommandPreprocessReadNV;
  static constexpr vk::AccessFlags s_writeAccess = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
    vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eTransferWrite |
    vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eMemoryWrite |
    vk::AccessFlagBits::eTransformFeedbackWriteEXT | vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT |
    vk::AccessFlagBits::eAccelerationStructureWriteKHR | vk::AccessFlagBits::eCommandPreprocessWriteNV;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;

  vk::PipelineStageFlags m_srcStageMask;
  vk::PipelineStageFlags m_dstStageMask;

  vk::AccessFlags m_srcAccess;
  vk::AccessFlags m_dstAccess;

  xiiHybridArray<vk::BufferMemoryBarrier, 8> m_bufferBarriers;
  xiiHybridArray<vk::ImageMemoryBarrier, 8>  m_imageBarriers;

  xiiMap<vk::Image, ImageState>   m_imageState;
  xiiMap<vk::Buffer, BufferState> m_bufferState;
};
