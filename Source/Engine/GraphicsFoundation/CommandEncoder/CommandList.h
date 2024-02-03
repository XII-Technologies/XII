#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief Interface that defines methods to manipulate a command list object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
  // State functions.

  void SetPipelineState(xiiGALPipelineStateHandle hPipelineState);

  void SetStencilRef(xiiUInt8 uiStencilRef);
  void SetBlendFactor(const xiiColor& blendFactor);

  void SetViewports(xiiArrayPtr<xiiRectFloat> pViewports, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRects(xiiArrayPtr<xiiRectU32> pRects);

  void SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt32 uiByteOffset = 0U);
  void SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt32> pByteOffsets);

  void ClearRenderTargetView(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor);
  void ClearDepthStencilView(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  // Draw functions.

  xiiResult Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);
  xiiResult DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex);
  xiiResult DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex);
  xiiResult DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  xiiResult DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex);
  xiiResult DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Dispatch functions.

  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);
  xiiResult DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Query functions.

  void BeginQuery(xiiGALQueryHandle hQuery);
  void EndQuery(xiiGALQueryHandle hQuery);

  // Buffer methods.

  void UpdateBuffer(xiiGALBufferHandle hBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard);
  void CopyBuffer(xiiGALBufferHandle hSourceBuffer, xiiGALBufferHandle hDestinationBuffer);
  void CopyBufferRegion(xiiGALBufferHandle hSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBufferHandle hDestinationBuffer, xiiUInt64 uiDestinationOffset);
  void MapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData);
  void UnmapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType);

  // Texture methods.

  void UpdateTexture(xiiGALTextureHandle hTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData);
  void CopyTexture(xiiGALTextureHandle hSourceTexture, xiiGALTextureHandle hDestinationTexture);
  void CopyTextureRegion(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint);
  void ResolveTextureSubResource(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData);
  void GenerateMips(xiiGALTextureViewHandle hTextureView);

  // Debug functions.

  void BeginDebugGroup(xiiStringView sName, const xiiColor& color = xiiColor::Black);
  void EndDebugGroup();
  void InsertDebugLabel(xiiStringView sName, const xiiColor& color = xiiColor::Black);

  void Flush();

  void InvalidateState();

public:
  void AssertRenderingThread() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALCommandList();

  virtual ~xiiGALCommandList();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a graphics API abstraction.
protected:
  virtual void SetPipelineStatePlatform(xiiGALPipelineStateHandle hPipelineState) = 0;

  virtual void SetStencilRefPlatform(xiiUInt8 uiStencilRef)        = 0;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) = 0;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiRectFloat> pViewports, float fMinDepth = 0.0f, float fMaxDepth = 1.0f) = 0;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)                                                    = 0;

  virtual void SetIndexBufferPlatform(xiiGALBufferHandle hIndexBuffer, xiiUInt32 uiByteOffset = 0U)                                                 = 0;
  virtual void SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt32> pByteOffsets) = 0;

  virtual void ClearRenderTargetViewPlatform(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor)                                                       = 0;
  virtual void ClearDepthStencilViewPlatform(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual xiiResult DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)                                                      = 0;
  virtual xiiResult DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)                                                 = 0;
  virtual xiiResult DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)  = 0;
  virtual xiiResult DrawIndexedInstancedIndirectPlatform(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) = 0;
  virtual xiiResult DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)       = 0;
  virtual xiiResult DrawInstancedIndirectPlatform(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)        = 0;

  virtual xiiResult DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) = 0;
  virtual xiiResult DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)                   = 0;

  virtual void BeginQueryPlatform(xiiGALQueryHandle hQuery) = 0;
  virtual void EndQueryPlatform(xiiGALQueryHandle hQuery)   = 0;

  virtual void UpdateBufferPlatform(xiiGALBufferHandle hBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard) = 0;
  virtual void CopyBufferPlatform(xiiGALBufferHandle hSourceBuffer, xiiGALBufferHandle hDestinationBuffer)                                                                                             = 0;
  virtual void CopyBufferRegionPlatform(xiiGALBufferHandle hSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBufferHandle hDestinationBuffer, xiiUInt64 uiDestinationOffset)                              = 0;
  virtual void MapBufferPlatform(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)                                                 = 0;
  virtual void UnmapBufferPlatform(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType)                                                                                                         = 0;

  virtual void UpdateTexturePlatform(xiiGALTextureHandle hTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)                                                                                      = 0;
  virtual void CopyTexturePlatform(xiiGALTextureHandle hSourceTexture, xiiGALTextureHandle hDestinationTexture)                                                                                                                                                                                         = 0;
  virtual void CopyTextureRegionPlatform(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) = 0;
  virtual void ResolveTextureSubResourcePlatform(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)                                                            = 0;
  virtual void GenerateMipsPlatform(xiiGALTextureViewHandle hTextureView)                                                                                                                                                                                                                               = 0;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color = xiiColor::Black)  = 0;
  virtual void EndDebugGroupPlatform()                                                                = 0;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color = xiiColor::Black) = 0;

  virtual void FlushPlatform() = 0;

  /// \endcond

protected:
  xiiGALPipelineStateHandle m_hPipelineState;
  xiiGALBufferHandle        m_hIndexBuffer;

  xiiGALRenderPassHandle m_hRenderPass;
  xiiGALFramebufferHandle m_hFramebuffer;


private:
  void CountDispatchCall();
  void CountDrawCall();

  // Statistic variables.
  xiiUInt32 m_uiDrawCalls     = 0U;
  xiiUInt32 m_uiDispatchCalls = 0U;

  #if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 m_uiDebugGroupCount = 0;
  #endif
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandList_inl.h>
