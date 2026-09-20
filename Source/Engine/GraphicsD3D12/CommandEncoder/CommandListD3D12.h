/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Algorithm/HashStream.h>

#include <GraphicsD3D12/CommandEncoder/CommandListDataD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <GraphicsD3D12/Pools/CommandListPoolD3D12.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;

class xiiGALFramebufferD3D12;
class xiiGALRenderPassD3D12;

class XII_GRAPHICSD3D12_DLL xiiGALCommandListD3D12 final : public xiiGALCommandList
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandListD3D12, xiiGALCommandList);

public:
  XII_ALWAYS_INLINE ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_pD3D12CommandList; }

  XII_ALWAYS_INLINE xiiGALStagingBufferPoolD3D12* GetD3D12UploadStagingBufferPool() const { return m_CommandListData.m_pUploadStagingBufferPool.Borrow(); }

  struct CommandListState
  {
    xiiUInt32 m_uiFramebufferWidth       = 0;
    xiiUInt32 m_uiFramebufferHeight      = 0;
    xiiUInt32 m_uiFramebufferArraySlices = 0;
    xiiUInt32 m_uiInsidePassQueries      = 0;
    xiiUInt32 m_uiOutsidePassQueries     = 0;
    bool      m_bIsShadingRateSet        = false;
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

  void FlushBarriers();

protected:
  friend class xiiGALCommandQueueD3D12;
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandListD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListD3D12();

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
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler) override final;
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
  struct MappedTextureKey
  {
    XII_DECLARE_POD_TYPE();

    xiiGALTextureD3D12* m_pTextureD3D12;
    xiiUInt32 const     m_uiMipLevel;
    xiiUInt32 const     m_uiArraySlice;

    bool operator==(const MappedTextureKey& rhs) const
    {
      return m_pTextureD3D12 == rhs.m_pTextureD3D12 && m_uiMipLevel == rhs.m_uiMipLevel && m_uiArraySlice == rhs.m_uiArraySlice;
    }

    struct Hasher
    {
      static xiiUInt32 Hash(const MappedTextureKey& key)
      {
        xiiHashStreamWriter32 writer;

        writer << key.m_pTextureD3D12;
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
    xiiGALDynamicBufferAllocationD3D12   m_DynamicAllocation;
  };

  struct MappedBufferKey
  {
    xiiGALBufferD3D12*     m_pBufferD3D12 = nullptr;
    xiiEnum<xiiGALMapType> m_MapType;

    bool operator==(const MappedBufferKey& rhs) const
    {
      return m_pBufferD3D12 == rhs.m_pBufferD3D12 && m_MapType == rhs.m_MapType;
    }

    struct Hasher
    {
      static xiiUInt32 Hash(const MappedBufferKey& key)
      {
        xiiHashStreamWriter32 writer;

        writer << key.m_pBufferD3D12;
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
    xiiEnum<xiiGALMapType>             m_MapType = xiiGALMapType::ENUM_COUNT;
    xiiGALDynamicBufferAllocationD3D12 m_DynamicAllocation;
  };

  struct FenceInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiGALFenceD3D12* m_pFenceD3D12 = nullptr;
    xiiUInt64         m_uiWaitValue = 0U;
  };

  void BindSubpassAttachments(xiiGALRenderPassD3D12* pRenderPassD3D12, xiiGALFramebufferD3D12* pFramebufferD3D12, xiiUInt32 uiSubpassIndex, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues);

  void PrepareForDraw();
  void PrepareForIndexedDraw(xiiEnum<xiiGALValueType> indexType);
  void PrepareForDispatchCompute();
  void PrepareForRayTracing();

public:
  void UpdateBufferRegion(xiiGALBufferD3D12* pBufferD3D12, ID3D12Resource* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSizeInBytes);

private:
  xiiGALCommandListPoolD3D12::AutoCommandList m_CommandListAllocation;
  ID3D12CommandAllocator*                     m_pD3D12CommandAllocator = nullptr;
  ID3D12GraphicsCommandList*                  m_pD3D12CommandList      = nullptr;
  xiiBitflags<CommandListFlags>               m_CommandListFlags;
  CommandListState                            m_CommandListState;
  xiiGALCommandListDataD3D12                  m_CommandListData;
  xiiDynamicArray<FenceInfo>                  m_SignalFences;
  xiiDynamicArray<FenceInfo>                  m_WaitFences;
  xiiUInt64                                   m_uiSubmittedFenceValue = 0ULL;

  xiiDynamicArray<D3D12_RESOURCE_BARRIER> m_PendingResourceBarriers;

  xiiHashTable<MappedBufferKey, MappedBuffer, MappedBufferKey::Hasher>    m_MappedBuffers;
  xiiHashTable<MappedTextureKey, MappedTexture, MappedTextureKey::Hasher> m_MappedTextures;
};
