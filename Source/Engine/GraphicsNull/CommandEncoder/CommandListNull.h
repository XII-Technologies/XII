#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSNULL_DLL xiiGALCommandListNull final : public xiiGALCommandList
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandListNull, xiiGALCommandList);

public:
protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceNull;
  friend class xiiGALCommandQueueNull;

  xiiGALCommandListNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, xiiGALCommandQueueNull* pCommandQueue, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListNull();

protected:
  virtual void BeginPlatform() override final;
  virtual void EndPlatform() override final;
  virtual void ResetPlatform() override final;

  virtual xiiUInt64 SubmitPlatform() override final;

  virtual void SetPipelineStatePlatform(xiiSharedPtr<xiiGALPipelineState> pPipelineState) override final;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef) override final;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) override final;

  virtual void      SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports) override final;
  virtual void      SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects) override final;
  virtual void      SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer) override final;
  virtual void      SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView) override final;
  virtual void      SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual void      SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView) override final;
  virtual void      SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual void      SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler) override final;
  virtual xiiResult CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode) override final;

  virtual void SetIndexBufferPlatform(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset) override final;
  virtual void SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags) override final;

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
  virtual void      ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData) override final;
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
};
