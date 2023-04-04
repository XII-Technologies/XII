
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class XII_RENDERERDILIGENT_DLL xiiGALCommandEncoderImplDiligent : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderRenderPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderImplDiligent(xiiGALDeviceDiligent& deviceDiligent);
  ~xiiGALCommandEncoderImplDiligent();

  // xiiGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const xiiGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void      BeginQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual void      EndQueryPlatform(const xiiGALQuery* pQuery) override;
  virtual xiiResult GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const xiiGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData) override;

  virtual void GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  // xiiGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const xiiGALRenderingSetup& renderingSetup);
  void EndRendering();

  // Draw functions

  virtual void ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;
  virtual void EndStreamOutPlatform() override;

  // State functions

  virtual void SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer) override;
  virtual void SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset) override;

  // xiiGALCommandEncoderComputePlatformInterface
  // Dispatch

  void BeginCompute();
  void EndCompute();

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

protected:
  void FlushDeferredStateChangesCompute();
  void FlushDeferredStateChangesGraphics();
  void FillDescriptorBindings(Diligent::IPipelineState* pPipelineState);

private:
  friend class xiiGALPassDiligent;
  friend class xiiGALSwapchainDiligent;

  xiiGALDeviceDiligent& m_GALDeviceDiligent;
  xiiGALCommandEncoder* m_pOwner = nullptr;

  Diligent::IDeviceContext* m_pContext;

  // Graphics pipeline state creation
  Diligent::GraphicsPipelineStateCreateInfo                 m_PipelineStateDesc;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState>         m_pPipelineStateGraphics;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pShaderResourceBindingGraphics;

  // Compute pipeline state creation
  Diligent::ComputePipelineStateCreateInfo                  m_PipelineStateComputeDesc;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState>         m_pPipelineStateCompute;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_pShaderResourceBindingCompute;

  // Pipeline State

  // Cache flags
  bool m_bPipelineStateModified = true;
  bool m_bViewportModified      = true;
  bool m_bIndexBufferModified   = false;
  bool m_bDescriptorsModified   = false;

  Diligent::Viewport m_Viewport;
  Diligent::Rect     m_ScissorRect;
  bool               m_bScissorEnabled = false;

  Diligent::PRIMITIVE_TOPOLOGY           m_PrimitiveTopology  = {};
  const xiiGALVertexDeclarationDiligent* m_pVertexDeclaration = nullptr;
  const xiiGALBlendStateDiligent*        m_pBlendStateState   = nullptr;
  const xiiGALDepthStencilStateDiligent* m_pDepthStencilState = nullptr;
  const xiiGALRasterizerStateDiligent*   m_pRasterizerState   = nullptr;

  // Bound objects for deferred state flushes
  xiiGALBufferDiligent* m_pIndexBuffer = nullptr;

  xiiGALBufferDiligent* m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};

  xiiHybridArray<xiiGALResourceViewDiligent*, 16> m_pBoundShaderResourceViews[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiGAL::ModifiedRange                           m_BoundShaderResourceViewsRange[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<xiiGALUnorderedAccessViewDiligent*, 16> m_pBoundUnoderedAccessViews;
  xiiGAL::ModifiedRange                                  m_pBoundUnoderedAccessViewsRange;

  xiiGALSamplerStateDiligent* m_pBoundSamplerStates[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};

  xiiGALShaderDiligent* m_pCurrentShader = nullptr;

  xiiGALRenderTargetSetup m_RenderTargetSetup;
  Diligent::ITextureView* m_pBoundRenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  xiiUInt8                m_uiBoundRenderTargetCount                            = 0;
  Diligent::ITextureView* m_pBoundDepthStencilTarget                            = nullptr;

  Diligent::IBuffer*    m_pBoundVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundVertexBuffersRange;

  Diligent::Uint64 m_VertexBufferStrides[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  Diligent::Uint64 m_VertexBufferOffsets[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
};
