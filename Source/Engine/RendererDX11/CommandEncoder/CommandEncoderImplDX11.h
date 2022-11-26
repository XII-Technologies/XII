
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

class xiiGALDeviceDX11;

class XII_RENDERERDX11_DLL xiiGALCommandEncoderImplDX11 : public xiiGALCommandEncoderCommonPlatformInterface, public xiiGALCommandEncoderRenderPlatformInterface, public xiiGALCommandEncoderComputePlatformInterface
{
public:
  xiiGALCommandEncoderImplDX11(xiiGALDeviceDX11& deviceDX11);
  ~xiiGALCommandEncoderImplDX11();

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
  void BeginCompute();

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

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) override;

private:
  friend class xiiGALPassDX11;

  void FlushDeferredStateChanges();

  xiiGALDeviceDX11&     m_GALDeviceDX11;
  xiiGALCommandEncoder* m_pOwner = nullptr;

  ID3D11DeviceContext*       m_pDXContext    = nullptr;
  ID3DUserDefinedAnnotation* m_pDXAnnotation = nullptr;

  // Bound objects for deferred state flushes
  ID3D11Buffer*         m_pBoundConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundConstantBuffersRange[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[xiiGALShaderStage::ENUM_COUNT] = {};
  xiiGAL::ModifiedRange                         m_BoundShaderResourceViewsRange[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnoderedAccessViews;
  xiiGAL::ModifiedRange                          m_BoundUnoderedAccessViewsRange;

  ID3D11SamplerState*   m_pBoundSamplerStates[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundSamplerStatesRange[xiiGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[xiiGALShaderStage::ENUM_COUNT] = {};

  xiiGALRenderTargetSetup m_RenderTargetSetup;
  ID3D11RenderTargetView* m_pBoundRenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  xiiUInt32               m_uiBoundRenderTargetCount                            = 0;
  ID3D11DepthStencilView* m_pBoundDepthStencilTarget                            = nullptr;

  ID3D11Buffer*         m_pBoundVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundVertexBuffersRange;

  xiiUInt32 m_VertexBufferStrides[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  xiiUInt32 m_VertexBufferOffsets[XII_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
};
