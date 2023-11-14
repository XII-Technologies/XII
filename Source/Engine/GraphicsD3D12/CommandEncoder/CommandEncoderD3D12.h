#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>

class XII_GRAPHICSD3D12_DLL xiiGALCommandEncoderD3D12 final : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderGraphicsPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12);
  ~xiiGALCommandEncoderD3D12();

  //  xiiGALCommandEncoderCommonPlatformInterface

  // State setting functions

  virtual void SetShaderPlatform(xiiGALShader* pShader) override;
  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer) override;
  virtual void SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler) override;
  virtual void SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView) override;
  virtual void SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pRTextureView) override;
  virtual void SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView) override;
  virtual void SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView) override;

  // Query functions

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) override;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery) override;

  // Fence functions

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues) override;

  virtual void CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) override;
  virtual void UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags) override;

  virtual void CopyTexturePlatform(xiiGALTexture* pDestination, xiiGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box) override;
  virtual void UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALMappedTextureSubresource& sourceData) override;
  virtual void ResolveTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource) override;
  virtual void ReadbackTexturePlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture) override;
  virtual void CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALMappedTextureSubresource> targetData) override;
  virtual void GenerateMipMapsPlatform(xiiGALTextureView* pTextureView) override;

  // Miscellaneous

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(xiiStringView sMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(xiiStringView sMarker) override;

  // xiiGALCommandEncoderGraphicsPlatformInterface

  // Draw functions

  virtual void ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

  // State functions

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer) override;
  virtual void SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout) override;
  virtual void SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology) override;

  virtual void SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect) override;

  // xiiGALCommandEncoderComputePlatformInterface

  // Dispatch

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

  // xiiGALCommandEncoderGraphicsPlatformInterface
  void BeginRendering(xiiGALRenderPassD3D12* pRenderPassD3D12, xiiGALFramebufferD3D12* pFramebufferD3D12);
  void EndRendering();

  // xiiGALCommandEncoderComputePlatformInterface

  void BeginCompute();
  void EndCompute();

private:
  friend class xiiGALPassD3D12;

  struct ShaderResourceViewDesc
  {
    enum Enum : xiiUInt8
    {
      Invalid,
      BufferView,
      TextureView
    };

    Enum                    m_Type = Invalid;
    xiiGALBufferViewD3D12*  m_pBufferView;
    xiiGALTextureViewD3D12* m_pTextureView;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const Diligent::GraphicsPipelineStateCreateInfo& desc);
    static bool      Equal(const Diligent::GraphicsPipelineStateCreateInfo& a, const Diligent::GraphicsPipelineStateCreateInfo& b);

    static xiiUInt32 Hash(const Diligent::ComputePipelineStateCreateInfo& desc);
    static bool      Equal(const Diligent::ComputePipelineStateCreateInfo& a, const Diligent::ComputePipelineStateCreateInfo& b);
  };

  struct PipelineStateInfo
  {
    Diligent::IPipelineState* m_pPipelineState = nullptr;
  };

  void FlushDeferredStateChanges();

  xiiGALDeviceD3D12&    m_GALDeviceD3D12;
  xiiGALCommandEncoder* m_pOwner = nullptr;

  Diligent::IDeviceContext* m_pContext = nullptr;

  // Render Pass and Framebuffer
  xiiGALRenderPassD3D12*  m_pRenderPass  = nullptr;
  xiiGALFramebufferD3D12* m_pFramebuffer = nullptr;

  // Pipeline state description
  xiiEnum<xiiGALPrimitiveTopology> m_PrimitiveTopology;
  xiiGALInputLayoutD3D12*          m_pInputLayout       = nullptr;
  xiiGALBlendStateD3D12*           m_pBlendState        = nullptr;
  xiiGALDepthStencilStateD3D12*    m_pDepthStencilState = nullptr;
  xiiGALRasterizerStateD3D12*      m_pRasterizerState   = nullptr;

  xiiHashTable<Diligent::GraphicsPipelineStateCreateInfo, PipelineStateInfo, ResourceCacheHash> m_CachedGraphicsPipelineStates;
  xiiHashTable<Diligent::ComputePipelineStateCreateInfo, PipelineStateInfo, ResourceCacheHash>  m_CachedComputePipelineStates;

  Diligent::IPipelineState*         m_pCurrentPipelineState         = nullptr;
  Diligent::IShaderResourceBinding* m_pCurrentShaderResourceBinding = nullptr;

  // Cache flags
  bool m_bPipelineStateModified = true;
  bool m_bViewportModified      = true;
  bool m_bIndexBufferModified   = false;
  bool m_bDescriptorsModified   = false;
  bool m_bRenderPassActive      = false;
  bool m_bIsComputeRequested    = false;
  bool m_bClearSubmitted        = false;

  // Viewport and Viewport scissor
  Diligent::Viewport m_Viewport        = {};
  Diligent::Rect     m_ScissorRect     = {};
  bool               m_bScissorEnabled = false;

  // Shader

  xiiGALShaderD3D12* m_pCurrentShader = nullptr;

  // Bound objects for deferred state flushes.
  xiiGALBufferD3D12*    m_pIndexBuffer                                         = nullptr;
  xiiGALBufferD3D12*    m_pBoundVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundVertexBuffersRange;

  xiiGALBufferD3D12*    m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundConstantBuffersRange[XII_GAL_MAX_CONSTANT_BUFFER_COUNT];

  xiiHybridArray<ShaderResourceViewDesc, 16U> m_pBoundShaderResourceViews[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiGAL::ModifiedRange                       m_BoundShaderResourceViewsRange[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<ShaderResourceViewDesc, 16U> m_pBoundUnorderedAccessViews;
  xiiGAL::ModifiedRange                       m_BoundUnorderedAccessViewsRange;

  xiiGALSamplerD3D12*   m_pBoundSamplers[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundSamplersRange[xiiGALShaderStage::ENUM_COUNT];

  // Synchronization fences.
  Diligent::RefCntAutoPtr<Diligent::IFence> m_pSynchronizationFence;
  xiiUInt64                                 m_uiSynchronizationFenceCompletedValue = 0u;
};

#include <GraphicsD3D12/CommandEncoder/Implementation/CommandEncoderD3D12_inl.h>
