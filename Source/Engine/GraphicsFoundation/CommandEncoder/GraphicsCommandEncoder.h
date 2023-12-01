#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsCommandEncoder : public xiiGALCommandEncoder
{
public:
  xiiGALGraphicsCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderGraphicsState& ref_graphicsState, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderGraphicsPlatformInterface& ref_graphicsImpl);

  virtual ~xiiGALGraphicsCommandEncoder();

public:
  // Draw functions

  void ClearRenderTarget(xiiGALTextureViewHandle hTextureView, const xiiColor& clearColor);
  void ClearDepthStencil(xiiGALTextureViewHandle hTextureView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  void Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);
  void DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex);
  void DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex);
  void DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  void DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex);
  void DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // State functions

  void SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt64 uiByteOffset);
  void SetVertexBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hVertexBuffer);
  void SetInputLayout(xiiGALInputLayoutHandle hInputLayout);

  void SetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> topology);

  void SetBlendState(xiiGALBlendStateHandle hBlendState, const xiiColor& blendFactor = xiiColor::White, xiiUInt32 uiSampleMask = 0xFFFFFFFFU);
  void SetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState, xiiUInt8 uiStencilRefValue = 0xFFU);
  void SetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const xiiRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const xiiRectU32& rect);

  virtual void ClearStatisticsCounters() override;

public:
  xiiEnum<xiiGALPrimitiveTopology> GetPrimitiveTopology();

private:
  void CountDrawCall();

  // Statistic variables
  xiiUInt32 m_uiDrawCalls = 0;

  xiiGALCommandEncoderGraphicsState& m_GraphicsState;

  xiiGALCommandEncoderGraphicsPlatformInterface& m_GraphicsImpl;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/GraphicsCommandEncoder_inl.h>
