
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoderCommonPlatformInterface
{
public:
  // State setting functions

  virtual void SetShaderPlatform(const xiiGALShader* pShader) = 0;

  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)                                          = 0;
  virtual void SetSamplerStatePlatform(xiiGALShaderStage::Enum stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState) = 0;
  virtual void SetResourceViewPlatform(xiiGALShaderStage::Enum stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView) = 0;
  virtual void SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)           = 0;

  // Query functions

  virtual void      BeginQueryPlatform(const xiiGALQuery* pQuery)                                   = 0;
  virtual void      EndQueryPlatform(const xiiGALQuery* pQuery)                                     = 0;
  virtual xiiResult GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& ref_uiQueryResult) = 0;

  // Timestamp functions

  virtual void InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp) = 0;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 vClearValues)    = 0;
  virtual void ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 vClearValues) = 0;

  virtual void CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource)                                                                                = 0;
  virtual void CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)                                                                                                                                                                                     = 0;
  virtual void CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& destinationSubResource, const xiiVec3U32& vDestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& sourceSubResource, const xiiBoundingBoxu32& box) = 0;

  virtual void UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALSystemMemoryDescription& sourceData) = 0;

  virtual void ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& destinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& sourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const xiiGALTexture* pTexture) = 0;

  virtual void CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> sourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> targetData) = 0;

  virtual void GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker)        = 0;
  virtual void PopMarkerPlatform()                             = 0;
  virtual void InsertEventMarkerPlatform(const char* szMarker) = 0;
};

class XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoderRenderPlatformInterface
{
public:
  // Draw functions

  virtual void ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)                                                       = 0;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)                                                  = 0;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)   = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)        = 0;
  virtual void DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)        = 0;
  virtual void DrawAutoPlatform()                                                                                                   = 0;

  virtual void BeginStreamOutPlatform() = 0;
  virtual void EndStreamOutPlatform()   = 0;

  // State functions

  virtual void SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)                        = 0;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)    = 0;
  virtual void SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration) = 0;
  virtual void SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum topology)            = 0;

  virtual void SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)     = 0;
  virtual void SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)                                       = 0;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect)                                  = 0;

  virtual void SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset) = 0;
};

class XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoderComputePlatformInterface
{
public:
  // Dispatch

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) = 0;
  virtual void DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)      = 0;
};
