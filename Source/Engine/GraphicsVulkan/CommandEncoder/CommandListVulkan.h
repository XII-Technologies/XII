/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>

#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <GraphicsVulkan/CommandEncoder/CommandListDataVulkan.h>

#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>

namespace vk
{
  class CommandBuffer;
  class RenderPass;
  class Framebuffer;
  class Pipeline;
  class Buffer;
  class Image;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALCommandListVulkan final : public xiiGALCommandList
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandListVulkan, xiiGALCommandList);

public:
  XII_ALWAYS_INLINE vk::CommandBuffer GetVulkanCommandBuffer() const { return m_vkCommandBuffer; }

  XII_ALWAYS_INLINE vk::PipelineStageFlags GetVulkanCommandBufferSupportedStageFlags() const { return m_PipelineBarrier.m_vkSupportedStageFlags; }

  XII_ALWAYS_INLINE vk::AccessFlags GetVulkanCommandBufferSupportedAccessFlags() const { return m_PipelineBarrier.m_vkSupportedAccessFlags; }

  void TransitionImageLayout(vk::Image vkImage, vk::ImageLayout vkOldLayout, vk::ImageLayout vkNewLayout, const vk::ImageSubresourceRange& vkImageSubresourceRange, vk::PipelineStageFlags vkPipelineSourceStageFlags, vk::PipelineStageFlags vkPipelineDestinationStageFlags);

  void MemoryBarrier(vk::AccessFlags vkSourceAccessFlags, vk::AccessFlags vkDestinationAccessFlags, vk::PipelineStageFlags vkPipelineSourceStageFlags, vk::PipelineStageFlags vkPipelineDestinationStageFlags);

  void TransitionBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, const bool bUpdateBufferState);
  void BufferMemoryBarrier(xiiGALBufferVulkan* pBufferVulkan, vk::AccessFlags newAccessFlags);

  void TransitionTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState, xiiBitflags<xiiGALStateTransitionFlags> flags, vk::ImageSubresourceRange* pSubresourceRange = nullptr);
  void TransitionImageLayout(xiiGALTextureVulkan* pTextureVulkan, vk::ImageLayout newLayout);

  void TransitionOrVerifyBufferState(xiiGALBufferVulkan* pBufferVulkan, xiiEnum<xiiGALStateTransitionMode> transitionMode, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::AccessFlagBits expectedAccessFlags, const char* szOperationName);
  void TransitionOrVerifyTextureState(xiiGALTextureVulkan* pTextureVulkan, xiiEnum<xiiGALStateTransitionMode> transitionMode, xiiBitflags<xiiGALResourceStateFlags> requiredState, vk::ImageLayout expectedLayout, const char* szOperationName);

  void FlushBarriers();

  void CopyBufferToTexture(vk::Buffer vkSourceBuffer, xiiUInt64 uiSourceBufferOffset, xiiUInt32 uiSourceBufferRowStrideInTexels, xiiGALTextureVulkan* pDestinationTextureVulkan, const xiiBoundingBoxU32& destinationRegion, xiiUInt32 uiDestinationMipLevel, xiiUInt32 uiDestinationArraySlice, bool bVerifyOnly = false);
  void CopyTextureToBuffer(xiiGALTextureVulkan* pSourceTextureVulkan, const xiiBoundingBoxU32& sourceRegion, xiiUInt32 uiSourceMipLevel, xiiUInt32 uiSourceArraySlice, vk::Buffer vkDestinationBuffer, xiiUInt64 uiDestinationBufferOffset, xiiUInt32 uiDestinationBufferRowStrideInTexels, bool bVerifyOnly = false);

  void UpdateBufferRegion(xiiGALBufferVulkan* pBufferVulkan, vk::Buffer vkSourceBuffer, xiiUInt64 uiSourceOffset, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSizeInBytes);

  void CopyBufferToImage(vk::Buffer vkSourceBuffer, vk::Image vkDestinationImage, vk::ImageLayout vkDestinationImageLayout, xiiArrayPtr<const vk::BufferImageCopy> pRegions);
  void CopyImageToBuffer(vk::Image vkSourceImage, vk::ImageLayout vkSourceImageLayout, vk::Buffer vkDestinationBuffer, xiiArrayPtr<const vk::BufferImageCopy> pRegions);
  void CopyImage(vk::Image vkSourceImage, vk::ImageLayout vkSourceImageLayout, vk::Image vkDestinationImage, vk::ImageLayout vkDestinationImageLayout, xiiArrayPtr<const vk::ImageCopy> pRegions);

  void CopyTextureRegion(xiiGALTextureVulkan* pSourceTextureVulkan, xiiGALTextureVulkan* pDestinationTextureVulkan, const vk::ImageCopy& copyRegion);

  void UpdateTextureRegion(const void* pSourceData, xiiUInt64 uiSourceStride, xiiUInt64 uiSourceDepthStride, xiiGALTextureVulkan* pTextureVulkan, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox);

  void AddWaitSemaphore(vk::Semaphore vkSemaphore, vk::PipelineStageFlags pipelineFlags, xiiUInt64 uiValue = 0ULL);
  void AddSignalSemaphore(vk::Semaphore vkSemaphore, xiiUInt64 uiValue = 0ULL);

  XII_ALWAYS_INLINE xiiGALStagingBufferPoolVulkan* GetVulkanUploadStagingBufferPool() const { return m_CommandListData.m_pUploadStagingBufferPool.Borrow(); }

  struct CommandListState
  {
    vk::RenderPass  m_vkRenderPass             = VK_NULL_HANDLE;
    vk::Framebuffer m_vkFramebuffer            = VK_NULL_HANDLE;
    vk::Pipeline    m_vkGraphicsPipeline       = VK_NULL_HANDLE;
    vk::Pipeline    m_vkComputePipeline        = VK_NULL_HANDLE;
    vk::Pipeline    m_vkRayTracingPipeline     = VK_NULL_HANDLE;
    vk::Buffer      m_vkIndexBuffer            = VK_NULL_HANDLE;
    vk::DeviceSize  m_vkIndexBufferOffset      = 0;
    vk::IndexType   m_vkIndexType              = vk::IndexType::eNoneKHR;
    xiiUInt32       m_uiFramebufferWidth       = 0;
    xiiUInt32       m_uiFramebufferHeight      = 0;
    xiiUInt32       m_uiFramebufferArraySlices = 0;
    xiiUInt32       m_uiInsidePassQueries      = 0;
    xiiUInt32       m_uiOutsidePassQueries     = 0;
    bool            m_bIsShadingRateSet        = false;
  };

  struct CommandListFlags
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      None                           = 0,
      CommittedVertexBuffersModified = XII_BIT(0),
      CommittedIndexBufferModified   = XII_BIT(1),
      ShadingRateSet                 = XII_BIT(2),

      Default = None
    };

    struct Bits
    {
      StorageType CommittedVertexBuffersModified : 1;
      StorageType CommittedIndexBufferModified : 1;
      StorageType ShadingRateSet : 1;
    };

    friend inline xiiBitflags<CommandListFlags> operator|(CommandListFlags::Enum lhs, CommandListFlags::Enum rhs)
    {
      return (xiiBitflags<CommandListFlags>(lhs) | xiiBitflags<CommandListFlags>(rhs));
    }
    friend inline xiiBitflags<CommandListFlags> operator&(CommandListFlags::Enum lhs, CommandListFlags::Enum rhs)
    {
      return (xiiBitflags<CommandListFlags>(lhs) & xiiBitflags<CommandListFlags>(rhs));
    };
  };

protected:
  friend class xiiGALCommandQueueVulkan;
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALCommandListVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListVulkan();

  virtual xiiResult InitPlatform() override final;

protected:
  virtual void BeginPlatform() override final;
  virtual void EndPlatform() override final;
  virtual void ResetPlatform() override final;

  virtual void SubmitPlatform(xiiGALCommandList* pSecondaryCommandList) override final;

  virtual void SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState) override final;
  virtual void PushConstantsPlatform(xiiUInt32 uiOffset, xiiArrayPtr<const xiiUInt8> pData) override final;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef) override final;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) override final;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports) override final;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects) override final;

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset, xiiEnum<xiiGALStateTransitionMode> transitionMode) override final;
  virtual void SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<VertexStreamDescription> pVertexStreams, xiiBitflags<xiiGALSetVertexBufferFlags> flags, xiiEnum<xiiGALStateTransitionMode> transitionMode) override final;

  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer) override final;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void      SetShaderResourceBufferViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews) override final;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetShaderResourceTextureViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews) override final;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void      SetUnorderedAccessBufferViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews) override final;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetUnorderedAccessTextureViewsPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews) override final;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler) override final;
  virtual void      SetSamplersPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers) override final;
  virtual void      SetAccelerationStructurePlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS) override final;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode) override final;

  virtual void ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor) override final;
  virtual void ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override final;

  virtual void BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) override final;
  virtual void NextSubpassPlatform() override final;
  virtual void EndRenderPassPlatform() override final;

  virtual void DrawPlatform(const xiiGALDrawDescription& description) override final;
  virtual void DrawIndexedPlatform(const xiiGALDrawIndexedDescription& description) override final;
  virtual void DrawIndirectPlatform(const xiiGALDrawIndirectDescription& description) override final;
  virtual void DrawIndexedIndirectPlatform(const xiiGALDrawIndexedIndirectDescription& description) override final;
  virtual void DrawMeshPlatform(const xiiGALDrawMeshDescription& description) override final;
  virtual void DrawMeshIndirectPlatform(const xiiGALDrawMeshIndirectDescription& description) override final;
  virtual void MultiDrawPlatform(const xiiGALMultiDrawDescription& description) override final;
  virtual void MultiDrawIndexedPlatform(const xiiGALMultiDrawIndexedDescription& description) override final;

  virtual void DispatchComputePlatform(const xiiGALDispatchComputeDescription& description) override final;
  virtual void DispatchComputeIndirectPlatform(const xiiGALDispatchComputeIndirectDescription& description) override final;
  virtual void TraceRaysPlatform(const xiiGALTraceRaysDescription& description) override final;
  virtual void TraceRaysIndirectPlatform(const xiiGALTraceRaysIndirectDescription& description) override final;
  virtual void UpdateSBTPlatform(const xiiGALUpdateSBTDescription& description) override final;
  virtual void BuildBLASPlatform(const xiiGALBuildBLASDescription& description) override final;
  virtual void BuildTLASPlatform(const xiiGALBuildTLASDescription& description) override final;
  virtual void CopyBLASPlatform(const xiiGALCopyBLASDescription& description) override final;
  virtual void CopyTLASPlatform(const xiiGALCopyTLASDescription& description) override final;
  virtual void WriteBLASCompactedSizePlatform(const xiiGALWriteBLASCompactedSizeDescription& description) override final;
  virtual void WriteTLASCompactedSizePlatform(const xiiGALWriteTLASCompactedSizeDescription& description) override final;

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) override final;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery) override final;

  virtual void      UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData) override final;
  virtual void      CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer) override final;
  virtual void      CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize) override final;
  virtual xiiResult MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData) override final;
  virtual xiiResult UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType) override final;

  virtual void      UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData) override final;
  virtual void      CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture) override final;
  virtual void      CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) override final;
  virtual void      ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description) override final;
  virtual void      GenerateMipsPlatform(xiiGALTextureView* pTextureView) override final;
  virtual xiiResult MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData) override final;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData) override final;

  virtual void SetShadingRatePlatform(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags) override final;

  virtual void TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers) override final;

  virtual void EnqueueSignalPlatform(xiiGALFence* pFence, xiiUInt64 uiValue) override final;
  virtual void DeviceWaitForFencePlatform(xiiGALFence* pFence, xiiUInt64 uiValue) override final;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color) override final;
  virtual void EndDebugGroupPlatform() override final;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) override final;

  virtual void InvalidateStatePlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  void PrepareForDraw();
  void PrepareForIndexedDraw(xiiEnum<xiiGALValueType> indexType);
  void PrepareForDispatchCompute();
  void PrepareForRayTracing();

private:
  struct PipelineBarrier
  {
    vk::PipelineStageFlags m_vkMemorySourceStages      = {};
    vk::PipelineStageFlags m_vkMemoryDestinationStages = {};
    vk::AccessFlags        m_vkMemorySourceAccess      = {};
    vk::AccessFlags        m_vkMemoryDestinationAccess = {};

    vk::PipelineStageFlags m_vkImageSourceStages      = {};
    vk::PipelineStageFlags m_vkImageDestinationStages = {};

    vk::PipelineStageFlags m_vkSupportedStageFlags  = {};
    vk::AccessFlags        m_vkSupportedAccessFlags = {};
  };

  struct MappedTextureKey
  {
    XII_DECLARE_POD_TYPE();

    xiiGALTextureVulkan* m_pTextureVulkan;
    xiiUInt32 const      m_uiMipLevel;
    xiiUInt32 const      m_uiArraySlice;

    bool operator==(const MappedTextureKey& rhs) const
    {
      return m_pTextureVulkan == rhs.m_pTextureVulkan && m_uiMipLevel == rhs.m_uiMipLevel && m_uiArraySlice == rhs.m_uiArraySlice;
    }

    struct Hasher
    {
      static xiiUInt32 Hash(const MappedTextureKey& key)
      {
        xiiHashStreamWriter32 writer;

        writer << key.m_pTextureVulkan;
        writer << key.m_uiMipLevel;
        writer << key.m_uiArraySlice;

        return writer.GetHashValue();
      }

      static bool Equal(const MappedTextureKey& a, const MappedTextureKey& b)
      {
        return a == b;
      }
    };
  };

  struct MappedTexture
  {
    xiiGALBufferToTextureCopyDescription m_CopyDescription;
    xiiGALDynamicBufferAllocationVulkan  m_DynamicAllocation;
  };

  struct MappedBufferKey
  {
    xiiGALBufferVulkan*    m_pBufferVulkan = nullptr;
    xiiEnum<xiiGALMapType> m_MapType;

    bool operator==(const MappedBufferKey& rhs) const
    {
      return m_pBufferVulkan == rhs.m_pBufferVulkan && m_MapType == rhs.m_MapType;
    }

    struct Hasher
    {
      static xiiUInt32 Hash(const MappedBufferKey& key)
      {
        xiiHashStreamWriter32 writer;

        writer << key.m_pBufferVulkan;
        writer << key.m_MapType;

        return writer.GetHashValue();
      }

      static bool Equal(const MappedBufferKey& a, const MappedBufferKey& b)
      {
        return a == b;
      }
    };
  };

  struct MappedBuffer
  {
    xiiEnum<xiiGALMapType>              m_MapType = xiiGALMapType::ENUM_COUNT;
    xiiGALDynamicBufferAllocationVulkan m_DynamicAllocation;
  };

  struct FenceInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiGALFenceVulkan* m_pFenceVulkan;
    xiiUInt64          m_uiWaitValue = 0U;
  };

  xiiGALCommandBufferPoolVulkan::AutoCommandBuffer m_CommandBufferAllocation;
  vk::CommandBuffer                                m_vkCommandBuffer;
  xiiBitflags<CommandListFlags>                    m_CommandListFlags;
  CommandListState                                 m_CommandListState;
  xiiGALCommandListDataVulkan                      m_CommandListData;

  PipelineBarrier                         m_PipelineBarrier;
  xiiDynamicArray<vk::ImageMemoryBarrier> m_ImageBarriers;

  xiiDynamicArray<vk::Semaphore>          m_vkWaitSemaphores;
  xiiDynamicArray<vk::Semaphore>          m_vkSignalSemaphores;
  xiiDynamicArray<vk::PipelineStageFlags> m_vkWaitDestinationStageFlags;

  // Can be used only if timeline semaphore extension is enabled.
  xiiDynamicArray<vk::DeviceSize> m_vkWaitSemaphoreValues;
  xiiDynamicArray<vk::DeviceSize> m_vkSignalSemaphoreValues;

  // List of fences to signal/wait next time the command queue is flushed.
  xiiDynamicArray<FenceInfo> m_SignalFences;
  xiiDynamicArray<FenceInfo> m_WaitFences;

  // Graphics/Mesh, Compute, Ray Tracing.
  static constexpr xiiUInt32 s_PipelineBindPointCount       = 3U;
  static constexpr xiiUInt32 s_MaxDescriptorSetPerSignature = 2U;

  xiiHashTable<MappedBufferKey, MappedBuffer, MappedBufferKey::Hasher>    m_MappedBuffers;
  xiiHashTable<MappedTextureKey, MappedTexture, MappedTextureKey::Hasher> m_MappedTextures;
};
