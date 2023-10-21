#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsCommandEncoder : public xiiGALCommandEncoder
{
public:
  xiiGALGraphicsCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderGraphicsState& ref_renderState, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderGraphicsPlatformInterface& ref_renderImpl);
  virtual ~xiiGALGraphicsCommandEncoder();

  // Draw functions

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask = 0xFFFFFFFFU, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, xiiUInt8 uiStencilClear = 0x0U);

  void Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);
  void DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex);
  void DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex);
  void DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  void DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex);
  void DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  void DrawAuto();

  void BeginStreamOut();
  void EndStreamOut();

  // State functions

  void SetIndexBuffer(xiiGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hVertexBuffer);
  void SetInputLayout(xiiGALInputLayoutHandle hInputLayout);

  xiiEnum<xiiGALPrimitiveTopology> GetPrimitiveTopology() const { return m_GraphicsState.m_Topology; }
  void                             SetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> topology);

  void SetBlendState(xiiGALBlendStateHandle hBlendState, const xiiColor& blendFactor = xiiColor::White, xiiUInt32 uiSampleMask = 0xFFFFFFFFU);
  void SetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState, xiiUInt8 uiStencilRefValue = 0xFFU);
  void SetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const xiiRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const xiiRectU32& rect);

  void SetStreamOutBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer, xiiUInt32 uiOffset);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  xiiUInt32 m_uiDrawCalls = 0;

  xiiGALCommandEncoderGraphicsState& m_GraphicsState;

  xiiGALCommandEncoderGraphicsPlatformInterface& m_GraphicsImpl;
};
