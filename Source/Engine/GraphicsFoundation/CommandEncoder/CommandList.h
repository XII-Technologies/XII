#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief This describes the pipeline state shading rate flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSetVertexBufferFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None  = 0U,         ///< No addditional operations.
    Reset = XII_BIT(1), ///< Reset the vertex buffers outside the range of the currently set vertex buffers. All buffers previously bound to the pipeline will be unbound.

    Default = None
  };

  struct Bits
  {
    StorageType Reset : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSetVertexBufferFlags);

/// \brief This describes the viewport.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALViewport : public xiiHashableStruct<xiiGALViewport>
{
  XII_DECLARE_POD_TYPE();

  float m_fTopLeftX = 0.0f;
  float m_fTopLeftY = 0.0f;
  float m_fWidth    = 0.0f;
  float m_fHeight   = 0.0f;
  float m_fMinDepth = 0.0f;
  float m_fMaxDepth = 1.0f;
};

/// \brief This describes the viewport.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBeginRenderPassDescription : public xiiHashableStruct<xiiGALBeginRenderPassDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiGALRenderPassHandle                                                    m_hRenderPass;
  xiiGALFramebufferHandle                                                   m_hFramebuffer;
  xiiStaticArray<xiiGALOptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT> m_ClearValues;
};

/// \brief This describes the command list creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListCreationDescription : public xiiHashableStruct<xiiGALCommandListCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                       m_sName;                                       ///< Resource name. The default is an empty string view.
  xiiBitflags<xiiGALCommandQueueType> m_QueueType = xiiGALCommandQueueType::Unknown; ///< The command queue type that this command list uses.
};

/// \brief Interface that defines methods to manipulate a command list object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
  void Begin();
  void End();
  void Reset();

  // State functions.

  void SetPipelineState(xiiGALPipelineStateHandle hPipelineState);

  void SetStencilRef(xiiUInt32 uiStencilRef);
  void SetBlendFactor(const xiiColor& blendFactor);

  void SetViewports(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight);
  void SetScissorRects(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight);

  void SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt64 uiByteOffset = 0U);
  void SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags = xiiGALSetVertexBufferFlags::None);

  void ClearRenderTargetView(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor);
  void ClearDepthStencilView(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  void BeginRenderPass(const xiiGALBeginRenderPassDescription& beginRenderPass);
  void NextSubpass();
  void EndRenderPass();

  /// \todo GraphicsFoundation: Add unordered access view clear.

  // Draw functions.

  xiiResult Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);
  xiiResult DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex);
  xiiResult DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance);
  xiiResult DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  xiiResult DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance);
  xiiResult DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  xiiResult DrawMesh(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);

  /// \todo GraphicsFoundation: Add indirect mesh draw via DrawIndirect().

  // Dispatch functions.

  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);
  xiiResult DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Query functions.

  void BeginQuery(xiiGALQueryHandle hQuery);
  void EndQuery(xiiGALQueryHandle hQuery);

  // Buffer methods.

  void      UpdateBuffer(xiiGALBufferHandle hBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard);
  void      CopyBuffer(xiiGALBufferHandle hSourceBuffer, xiiGALBufferHandle hDestinationBuffer);
  void      CopyBufferRegion(xiiGALBufferHandle hSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBufferHandle hDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize);
  xiiResult MapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData);
  xiiResult UnmapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType);

  // Texture methods.

  void      UpdateTexture(xiiGALTextureHandle hTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData);
  void      CopyTexture(xiiGALTextureHandle hSourceTexture, xiiGALTextureHandle hDestinationTexture);
  void      CopyTextureRegion(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint);
  void      ResolveTextureSubResource(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData);
  void      GenerateMips(xiiGALTextureViewHandle hTextureView);
  xiiResult MapTextureSubresource(xiiGALTextureHandle hTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData);
  xiiResult UnmapTextureSubresource(xiiGALTextureHandle hTexture, xiiGALTextureMipLevelData textureMipLevelData);

  // Debug functions.

  void BeginDebugGroup(xiiStringView sName, const xiiColor& color = xiiColor::Black);
  void EndDebugGroup();
  void InsertDebugLabel(xiiStringView sName, const xiiColor& color = xiiColor::Black);

  void Flush();

  void InvalidateState();

public:
  enum class RecordingState
  {
    Recording,
    Ended,
    Reset
  };

  void AssertRenderingThread() const;

  RecordingState GetRecordingState() const;

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALCommandList(xiiGALDevice* pDevice, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandList();

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a graphics API abstraction.
protected:
  virtual void BeginPlatform() = 0;
  virtual void EndPlatform()   = 0;
  virtual void ResetPlatform() = 0;

  virtual void SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState) = 0;

  virtual void SetStencilRefPlatform(xiiUInt32 uiStencilRef)       = 0;
  virtual void SetBlendFactorPlatform(const xiiColor& blendFactor) = 0;

  virtual void SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight) = 0;
  virtual void SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)      = 0;

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)                                                                                                     = 0;
  virtual void SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags) = 0;

  virtual void ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)                                                       = 0;
  virtual void ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual void BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues) = 0;
  virtual void NextSubpassPlatform()                                                                                                                                       = 0;
  virtual void EndRenderPassPlatform()                                                                                                                                     = 0;

  virtual xiiResult DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)                                                                                                        = 0;
  virtual xiiResult DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)                                                                           = 0;
  virtual xiiResult DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance) = 0;
  virtual xiiResult DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)                                                        = 0;
  virtual xiiResult DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)                              = 0;
  virtual xiiResult DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)                                                               = 0;
  virtual xiiResult DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)                                                         = 0;

  virtual xiiResult DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) = 0;
  virtual xiiResult DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)            = 0;

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery)   = 0;

  virtual void      UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)          = 0;
  virtual void      CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)                                                                                  = 0;
  virtual void      CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize) = 0;
  virtual xiiResult MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)                                 = 0;
  virtual xiiResult UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)                                                                                         = 0;

  virtual void      UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)                                                                                 = 0;
  virtual void      CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)                                                                                                                                                                                         = 0;
  virtual void      CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint) = 0;
  virtual void      ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)                                                            = 0;
  virtual void      GenerateMipsPlatform(xiiGALTextureView* pTextureView)                                                                                                                                                                                                                          = 0;
  virtual xiiResult MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData)                          = 0;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)                                                                                                                                                                        = 0;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)  = 0;
  virtual void EndDebugGroupPlatform()                                              = 0;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) = 0;

  virtual void FlushPlatform() = 0;

  /// \endcond

protected:
  xiiGALCommandListCreationDescription m_Description;

  RecordingState m_RecordingState = RecordingState::Reset;

  xiiGALPipelineStateHandle             m_hPipelineState;
  xiiGALPipelineResourceSignatureHandle m_hPipelineResourceSignature;

  xiiGALBufferHandle m_VertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  xiiGALBufferHandle m_hIndexBuffer;
  xiiUInt64          m_uiIndexDataOffset = 0ULL;

  xiiGALRenderPassHandle  m_hRenderPass;
  xiiGALFramebufferHandle m_hFramebuffer;

  xiiColor  m_BlendFactors = xiiColor::Black;
  xiiUInt32 m_uiStencilRef = 0U;

  xiiHybridArray<xiiGALViewport, 2U> m_Viewports;
  xiiHybridArray<xiiRectU32, 2U>     m_ScissorRects;

private:
  void CountDispatchCall();
  void CountDrawCall();
  void ClearStatisticCounters();

  // Statistic variables.
  xiiUInt32 m_uiDrawCalls     = 0U;
  xiiUInt32 m_uiDispatchCalls = 0U;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 m_uiDebugGroupCount = 0;

  xiiMap<xiiUInt32, xiiEnum<xiiGALMapType>> m_MappedBuffers;
#endif
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandList_inl.h>
