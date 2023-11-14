#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// \brief The command encoder common platform interface.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderCommonPlatformInterface
{
public:
  // State setting functions

  virtual void SetShaderPlatform(xiiGALShader* pShader)                                                                         = 0;
  virtual void SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer)                                               = 0;
  virtual void SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler)              = 0;
  virtual void SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView)     = 0;
  virtual void SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pRTextureView) = 0;
  virtual void SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView)             = 0;
  virtual void SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView)          = 0;

  // Query functions

  virtual void BeginQueryPlatform(xiiGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(xiiGALQuery* pQuery)   = 0;

  // Fence functions

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues)      = 0;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues)    = 0;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues)   = 0;
  virtual void ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues) = 0;

  virtual void CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource)                                                                                  = 0;
  virtual void CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)   = 0;
  virtual void UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags) = 0;

  virtual void CopyTexturePlatform(xiiGALTexture* pDestination, xiiGALTexture* pSource)                                                                                                                                                                                       = 0;
  virtual void CopyTextureRegionPlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box) = 0;
  virtual void UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALMappedTextureSubresource& sourceData)                                                         = 0;
  virtual void ResolveTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource)                                                                       = 0;
  virtual void ReadbackTexturePlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture)                                                                                                                                                                               = 0;
  virtual void CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALMappedTextureSubresource> targetData)                                                        = 0;
  virtual void GenerateMipMapsPlatform(xiiGALTextureView* pTextureView)                                                                                                                                                                                                       = 0;

  // Miscellaneous

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(xiiStringView sMarker)        = 0;
  virtual void PopMarkerPlatform()                              = 0;
  virtual void InsertEventMarkerPlatform(xiiStringView sMarker) = 0;
};

/// \brief The command encoder graphics platform interface.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderGraphicsPlatformInterface
{
public:
  // Draw functions

  virtual void ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear) = 0;

  virtual void DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)                                                     = 0;
  virtual void DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)                                                = 0;
  virtual void DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)     = 0;
  virtual void DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)      = 0;
  virtual void DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)            = 0;

  // State functions

  virtual void SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer)                      = 0;
  virtual void SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer)  = 0;
  virtual void SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout)                 = 0;
  virtual void SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology) = 0;

  virtual void SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)     = 0;
  virtual void SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)                                       = 0;

  virtual void SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const xiiRectU32& rect)                                  = 0;
};

/// \brief The command encoder compute platform interface.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderComputePlatformInterface
{
public:
  // Dispatch

  virtual void DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ) = 0;
  virtual void DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)            = 0;
};
