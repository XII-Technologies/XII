#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

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

  void AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags pipelineFlags, xiiUInt64 uiValue = 0ULL);
  void AddSignalSemaphore(vk::Semaphore semaphore, xiiUInt64 uiValue = 0ULL);

  XII_ALWAYS_INLINE xiiGALStagingBufferPoolVulkan* GetVulkanUploadStagingBufferPool() const { return m_pUploadStagingBufferPool.Borrow(); }

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
  };

protected:
  friend class xiiGALCommandQueueVulkan;
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALCommandListVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, xiiGALCommandBufferPoolVulkan* pCommandBufferPool, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListVulkan();

protected:
  virtual void BeginPlatform() override final;
  virtual void EndPlatform() override final;
  virtual void ResetPlatform() override final;

  virtual xiiUInt64 SubmitPlatform() override final;

  virtual void SetPipelineStatePlatform(xiiSharedPtr<xiiGALPipelineState> pPipelineState) override final;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef) override final;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) override final;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports) override final;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects) override final;

  virtual void      SetIndexBufferPlatform(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset) override final;
  virtual void      SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags) override final;
  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer) override final;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView) override final;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView) override final;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler) override final;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode) override final;

  virtual void ClearRenderTargetViewPlatform(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor) override final;
  virtual void ClearDepthStencilViewPlatform(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override final;

  virtual void BeginRenderPassPlatform(xiiSharedPtr<xiiGALRenderPass> pRenderPass, xiiSharedPtr<xiiGALFramebuffer> pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) override final;
  virtual void NextSubpassPlatform() override final;
  virtual void EndRenderPassPlatform() override final;

  virtual xiiResult DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override final;
  virtual xiiResult DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex) override final;
  virtual xiiResult DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance) override final;
  virtual xiiResult DrawIndexedInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;
  virtual xiiResult DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance) override final;
  virtual xiiResult DrawInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;
  virtual xiiResult DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override final;

  virtual xiiResult DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override final;
  virtual xiiResult DispatchIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;

  virtual void BeginQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery) override final;
  virtual void EndQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery) override final;

  virtual void      UpdateBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData) override final;
  virtual void      CopyBufferPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer) override final;
  virtual void      CopyBufferRegionPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize) override final;
  virtual xiiResult MapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData) override final;
  virtual xiiResult UnmapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType) override final;

  virtual void      UpdateTexturePlatform(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData) override final;
  virtual void      CopyTexturePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture) override final;
  virtual void      CopyTextureRegionPlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) override final;
  virtual void      ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description) override final;
  virtual void      GenerateMipsPlatform(xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual xiiResult MapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData) override final;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData) override final;

  virtual void TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers) override final;

  virtual void EnqueueSignalPlatform(xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue) override final;
  virtual void DeviceWaitForFencePlatform(xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue) override final;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color) override final;
  virtual void EndDebugGroupPlatform() override final;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) override final;

  virtual void InvalidateStatePlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

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
    xiiSharedPtr<xiiGALTextureVulkan> m_pTextureVulkan;
    xiiUInt32 const                   m_uiMipLevel;
    xiiUInt32 const                   m_uiArraySlice;

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
    xiiSharedPtr<xiiGALBufferVulkan> m_pBufferVulkan = nullptr;
    xiiEnum<xiiGALMapType>           m_MapType;

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
    xiiSharedPtr<xiiGALFenceVulkan> m_pFenceVulkan;
    xiiUInt64                       m_uiWaitValue = 0U;
  };

  struct ResourceSetBindings
  {
    xiiDynamicArray<xiiSharedPtr<xiiGALBufferVulkan>>      m_pBoundConstantBuffers;
    xiiDynamicArray<xiiSharedPtr<xiiGALBufferViewVulkan>>  m_pBoundBufferResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureViewVulkan>> m_pBoundTextureResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALBufferViewVulkan>>  m_pBoundUnorderedAccessBufferResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureViewVulkan>> m_pBoundUnorderedAccessTextureResourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALSamplerVulkan>>     m_pBoundSamplerStates;
  };

  xiiGALCommandBufferPoolVulkan* m_pCommandBufferPool;

  vk::CommandBuffer m_vkCommandBuffer;
  CommandListState  m_CommandListState;
  PipelineBarrier   m_PipelineBarrier;

  xiiDynamicArray<vk::ImageMemoryBarrier> m_ImageBarriers;

  xiiSharedPtr<xiiGALTextureViewVulkan> m_pBoundRenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  xiiSharedPtr<xiiGALTextureViewVulkan> m_pBoundDepthStencilTarget;
  xiiUInt32                             m_uiBoundRenderTargetCount = 0U;

  xiiUInt32                                                          m_uiSubpassIndex = 0U;
  xiiStaticArray<vk::ClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_AttachmentClearValues;

  bool m_bPipelineStateModified = false;

  xiiHybridArray<ResourceSetBindings, 1U> m_ResourceSets;
  xiiHybridArray<vk::DescriptorSet, 4U>   m_DescriptorSets;
  xiiDeque<vk::DescriptorBufferInfo>      m_DynamicUniformBuffers;
  xiiHybridArray<xiiUInt32, 6U>           m_DynamicUniformBufferOffsets;
  bool                                    m_bDescriptorsModified = false;

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

  xiiUniquePtr<xiiGALDynamicBufferPoolVulkan> m_pDynamicBufferPoolVulkan;
  xiiUniquePtr<xiiGALStagingBufferPoolVulkan> m_pUploadStagingBufferPool;

  xiiUInt32 m_uiActiveQueriesCounter = 0U;
};
