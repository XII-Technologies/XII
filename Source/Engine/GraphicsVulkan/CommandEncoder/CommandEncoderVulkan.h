#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <GraphicsFoundation/Resources/RenderTargetSetup.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandEncoderVulkan final : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderGraphicsPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderVulkan(xiiGALDeviceVulkan& deviceVulkan);
  ~xiiGALCommandEncoderVulkan();

  //  xiiGALCommandEncoderCommonPlatformInterface

  // State setting functions

  virtual void SetShaderPlatform(xiiGALShader* pShader) override final;
  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer) override final;
  virtual void SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler) override final;
  virtual void SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView) override final;
  virtual void SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pTextureView) override final;
  virtual void SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView) override final;
  virtual void SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView) override final;

  // Query functions

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) override final;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery) override final;

  // Fence functions

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues) override final;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues) override final;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues) override final;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues) override final;

  virtual void CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource) override final;
  virtual void CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) override final;
  virtual void UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags) override final;

  virtual void CopyTexturePlatform(xiiGALTexture* pDestination, xiiGALTexture* pSource) override final;
  virtual void CopyTextureRegionPlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box) override final;
  virtual void UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureSubResourceData& sourceData) override final;
  virtual void ResolveTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource) override final;
  virtual void ReadbackTexturePlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture) override final;
  virtual void CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALTextureSubResourceData> targetData) override final;
  virtual void GenerateMipMapsPlatform(xiiGALTextureView* pTextureView) override final;

  // Miscellaneous

  virtual void FlushPlatform() override final;

  // Debug helper functions

  virtual void PushMarkerPlatform(xiiStringView sMarker) override final;
  virtual void PopMarkerPlatform() override final;
  virtual void InsertEventMarkerPlatform(xiiStringView sMarker, const xiiColor& color) override final;

  // xiiGALCommandEncoderGraphicsPlatformInterface

  // Draw functions

  virtual void ClearRenderTargetPlatform(xiiGALTextureView* pTextureView, const xiiColor& clearColor) override final;
  virtual void ClearDepthStencilPlatform(xiiGALTextureView* pTextureView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) override final;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex) override final;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex) override final;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) override final;
  virtual void DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex) override final;
  virtual void DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;

  // State functions

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset) override final;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer) override final;
  virtual void SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout) override final;
  virtual void SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology) override final;

  virtual void SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask) override final;
  virtual void SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue) override final;
  virtual void SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override final;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) override final;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect) override final;

  // xiiGALCommandEncoderComputePlatformInterface

  // Dispatch

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override final;
  virtual void DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override final;

  // xiiGALCommandEncoderGraphicsPlatformInterface
  void BeginRendering(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPassVulkan* pRenderPassVulkan, xiiGALFramebufferVulkan* pFramebufferVulkan);
  void EndRendering();

  // xiiGALCommandEncoderComputePlatformInterface

  void BeginCompute();
  void EndCompute();

  void Reset();

private:
  friend class xiiGALPassVulkan;

  struct ShaderResourceViewDesc
  {
    XII_DECLARE_POD_TYPE();

    enum Enum : xiiUInt8
    {
      Invalid,
      BufferView,
      TextureView
    };

    Enum                    m_Type = Invalid;
    Diligent::IBufferView*  m_pBufferView;
    Diligent::ITextureView* m_pTextureView;
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
    XII_DECLARE_POD_TYPE();

    xiiGALShaderVulkan*               m_pShader                = nullptr;
    Diligent::IPipelineState*         m_pPipelineState         = nullptr;
    Diligent::IShaderResourceBinding* m_pShaderResourceBinding = nullptr;
  };

  void FlushDeferredStateChanges();

  void FlushPipelineStateCache();

  void BeginRenderPass();
  void EndRenderPass();

  xiiGALDeviceVulkan&   m_GALDeviceVulkan;
  xiiGALCommandEncoder* m_pOwner = nullptr;

  Diligent::IDeviceContext* m_pContext = nullptr;

  // Render Pass and Framebuffer
  xiiGALRenderPassVulkan*  m_pRenderPass    = nullptr;
  xiiGALFramebufferVulkan* m_pFramebuffer   = nullptr;
  xiiGALRenderingSetup     m_RenderingSetup = {};

  // Pipeline state description
  xiiEnum<xiiGALPrimitiveTopology> m_PrimitiveTopology;
  xiiGALShaderVulkan*              m_pCurrentShader     = nullptr;
  xiiGALInputLayoutVulkan*         m_pInputLayout       = nullptr;
  xiiGALBlendStateVulkan*          m_pBlendState        = nullptr;
  xiiGALDepthStencilStateVulkan*   m_pDepthStencilState = nullptr;
  xiiGALRasterizerStateVulkan*     m_pRasterizerState   = nullptr;

  xiiHashTable<Diligent::GraphicsPipelineStateCreateInfo, PipelineStateInfo, ResourceCacheHash> m_CachedGraphicsPipelineStates;
  xiiHashTable<Diligent::ComputePipelineStateCreateInfo, PipelineStateInfo, ResourceCacheHash>  m_CachedComputePipelineStates;
  xiiHashTable<xiiGALShaderVulkan*, xiiEventSubscriptionID>                                     m_CachedShaderEventIDs;

  Diligent::IPipelineState*         m_pCurrentPipelineState         = nullptr;
  Diligent::IShaderResourceBinding* m_pCurrentShaderResourceBinding = nullptr;

  // Cache flags
  bool m_bPipelineStateModified = true;
  bool m_bIndexBufferModified   = false;
  bool m_bDescriptorsModified   = false;
  bool m_bViewportModified      = true;
  bool m_bRenderPassActive      = false;
  bool m_bIsComputeRequested    = false;
  bool m_bClearSubmitted        = false;

  Diligent::Viewport m_Viewport        = {};
  Diligent::Rect     m_ScissorRect     = {};
  bool               m_bScissorEnabled = false;

  // Bound objects for deferred state flushes.
  Diligent::VALUE_TYPE  m_IndexFormat                                          = {};
  xiiUInt64             m_uiIndexBufferByteOffset                              = 0U;
  Diligent::IBuffer*    m_pIndexBuffer                                         = nullptr;
  Diligent::IBuffer*    m_pBoundVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundVertexBuffersRange;

  xiiUInt32        m_VertexBufferStrides[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  Diligent::Uint64 m_VertexBufferOffsets[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  Diligent::IBuffer*                          m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiHybridArray<ShaderResourceViewDesc, 16U> m_pBoundShaderResourceViews[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiHybridArray<ShaderResourceViewDesc, 16U> m_pBoundUnorderedAccessViews;
  Diligent::ISampler*                         m_pBoundSamplers[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};

  // Synchronization fences.
  Diligent::IFence* m_pSynchronizationFence                = nullptr;
  xiiUInt64         m_uiSynchronizationFenceCompletedValue = 0u;
};

#include <GraphicsVulkan/CommandEncoder/Implementation/CommandEncoderVulkan_inl.h>
