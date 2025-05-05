#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

#include <GraphicsD3D11/Resources/DisjointQueryPool.h>
#include <GraphicsD3D11/States/PipelineStateD3D11.h>

struct ID3D11DeviceContext;
struct ID3D11CommandList;
struct xiiGALSwapChainD3D11Event;

class XII_GRAPHICSD3D11_DLL xiiGALCommandListD3D11 final : public xiiGALCommandList
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandListD3D11, xiiGALCommandList);

public:
  XII_ALWAYS_INLINE ID3D11DeviceContext4* GetD3D11Context() const { return m_pImmediateContext; }

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
  virtual void      ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData) override final;
  virtual void      GenerateMipsPlatform(xiiSharedPtr<xiiGALTextureView> pTextureView) override final;
  virtual xiiResult MapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData) override final;
  virtual xiiResult UnmapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData) override final;

  virtual void BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color) override final;
  virtual void EndDebugGroupPlatform() override final;
  virtual void InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color) override final;

  virtual void InvalidateStatePlatform() override final;

protected:
  friend class xiiGALCommandQueueD3D11;
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALCommandListD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, xiiGALCommandQueueD3D11* pCommandQueueD3D11, const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListD3D11();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  void GALSwapChainD3D11EventHandler(const xiiGALSwapChainD3D11Event& e);

  void InvalidateCommittedResources();

  void CommitRenderTargets();

  bool UnsetResourceViews(const xiiSharedPtr<xiiGALResource> pResource);
  bool UnsetUnorderedAccessViews(const xiiSharedPtr<xiiGALResource> pResource);

  xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> BeginDisjointQuery();

  xiiResult FlushDeferredStateChanges();

private:
  xiiEventSubscriptionID m_GALSwapChainD3D11EventSubscriptionID;

  xiiGALCommandQueueD3D11* m_pCommandQueueD3D11 = nullptr;

  ID3D11DeviceContext4* m_pImmediateContext = nullptr;

  // Deferred state flushes flags
  bool m_bIndexBufferModified       = false;
  bool m_bBlendStateModified        = false;
  bool m_bInputLayoutStateModified  = false;
  bool m_bDepthStencilStateModified = false;
  bool m_bRasterizerStateModified   = false;
  bool m_bPrimitiveTopologyModified = false;

  // Bound objects for deferred state flushes

  ID3D11Buffer*         m_pCommittedVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT]      = {};
  xiiUInt32             m_CommittedVertexBufferStrides[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiUInt32             m_CommittedVertexBufferOffsets[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_CommittedVertexBuffersRange;

  ID3D11Buffer* m_pCommittedIndexBuffer           = nullptr;
  DXGI_FORMAT   m_CommittedIndexBufferFormat      = DXGI_FORMAT_R16_UINT;
  xiiUInt32     m_uiCommittedIndexDataStartOffset = 0;

  ID3D11InputLayout*       m_pCommittedInputLayout       = nullptr;
  ID3D11RasterizerState*   m_pCommittedRasterizerState   = nullptr;
  ID3D11BlendState*        m_pCommittedBlendState        = nullptr;
  ID3D11DepthStencilState* m_pCommittedDepthStencilState = nullptr;

  D3D11_PRIMITIVE_TOPOLOGY m_CommittedPrimitiveTopology  = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  xiiColor                 m_CommittedBlendFactors       = xiiColor::White;
  xiiUInt32                m_uiCommittedBlendSampleMask  = 0xFFFFFFFFU;
  xiiUInt32                m_uiCommittedStencilReference = 0x0;

  ID3D11RenderTargetView* m_pCommittedRenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  ID3D11DepthStencilView* m_pCommittedDepthStencilTarget                            = nullptr;
  xiiUInt32               m_uiBoundRenderTargetCount                                = 0U;

  xiiUInt32                                                                     m_uiSubpassIndex = 0U;
  xiiGALRenderPass*                                                             m_pRenderPass    = nullptr;
  xiiGALFramebuffer*                                                            m_pFramebuffer   = nullptr;
  xiiStaticArray<xiiGALOptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_AttachmentClearValues;

  ID3D11Buffer*         m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundConstantBuffersRange[xiiGALD3D11ShaderType::ENUM_COUNT];

  xiiHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[xiiGALD3D11ShaderType::ENUM_COUNT] = {};
  xiiHybridArray<const xiiGALResource*, 16>     m_ResourcesForResourceViews[xiiGALD3D11ShaderType::ENUM_COUNT];
  xiiGAL::ModifiedRange                         m_BoundShaderResourceViewsRange[xiiGALD3D11ShaderType::ENUM_COUNT];

  xiiHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnorderedAccessViews;
  xiiHybridArray<const xiiGALResource*, 16>      m_ResourcesForUnorderedAccessViews;
  xiiGAL::ModifiedRange                          m_BoundUnorderedAccessViewsRange;

  ID3D11SamplerState*   m_pBoundSamplerStates[xiiGALD3D11ShaderType::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundSamplerStatesRange[xiiGALD3D11ShaderType::ENUM_COUNT];

  ID3D11DeviceChild* m_CommittedShaders[xiiGALD3D11ShaderType::ENUM_COUNT]                  = {};
  bool               m_CommittedShaderModificationStates[xiiGALD3D11ShaderType::ENUM_COUNT] = {};

  xiiMap<xiiGALBufferD3D11*, ID3D11DeviceContext4*>  m_MappedBuffers;
  xiiMap<xiiGALTextureD3D11*, ID3D11DeviceContext4*> m_MappedTextureSubresources;

  xiiDisjointQueryPool                                     m_DisjointQueryPool;
  xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> m_pActiveDisjointQuery;
};
