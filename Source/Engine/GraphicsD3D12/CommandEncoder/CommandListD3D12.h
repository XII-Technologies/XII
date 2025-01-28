#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandListD3D12 final : public xiiGALCommandList
{
public:
protected:
  friend class xiiGALCommandQueueD3D12;
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALCommandListD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListD3D12();

protected:
  virtual void BeginPlatform() override final;
  virtual void EndPlatform() override final;
  virtual void ResetPlatform() override final;

  virtual xiiUInt64 SubmitPlatform() override final;

  virtual void SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState) override final;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef) override final;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) override final;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight) override final;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight) override final;

  virtual void      SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset) override final;
  virtual void      SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags) override final;
  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer) override final;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler) override final;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode) override final;

  virtual void ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor) override final;
  virtual void ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override final;

  virtual void BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) override final;
  virtual void NextSubpassPlatform() override final;
  virtual void EndRenderPassPlatform() override final;

  virtual xiiResult DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override final;
  virtual xiiResult DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex) override final;
  virtual xiiResult DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance) override final;
  virtual xiiResult DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;
  virtual xiiResult DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance) override final;
  virtual xiiResult DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;
  virtual xiiResult DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override final;

  virtual xiiResult DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override final;
  virtual xiiResult DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;

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
  virtual void      ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData) override final;
  virtual void      GenerateMipsPlatform(xiiGALTextureView* pTextureView) override final;
  virtual xiiResult MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData) override final;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData) override final;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color) override final;
  virtual void EndDebugGroupPlatform() override final;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) override final;

  virtual void InvalidateStatePlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  static constexpr xiiUInt32 s_uiCommandAllocatorDesiredCount = 2U;

  struct FenceData
  {
    ID3D12Fence* m_pD3D12Fence         = nullptr;
    xiiUInt64    m_uiCurrentFenceValue = 0U;
    xiiUInt64    m_uiNextFenceValue    = 1U;
  };

  struct StateCache
  {
    ID3D12RootSignature* m_pD3D12GraphicsRootSignature = nullptr;
    ID3D12RootSignature* m_pD3D12ComputeRootSignature  = nullptr;
    ID3D12PipelineState* m_pD3D12PipelineState         = nullptr;
  };

  ID3D12CommandQueue*                                                       m_pD3D12CommandQueue;
  ID3D12GraphicsCommandList*                                                m_pD3D12CommandList;
  xiiUInt32                                                                 m_uiCurrentAllocatorIndex;
  xiiHybridArray<ID3D12CommandAllocator*, s_uiCommandAllocatorDesiredCount> m_D3D12CommandAllocators;
  xiiHybridArray<FenceData, s_uiCommandAllocatorDesiredCount>               m_D3D12CommandAllocatorFenceData;

  StateCache m_StateCache;
};
