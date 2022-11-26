
#pragma once

#include <Foundation/Math/Rect.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class XII_RENDERERFOUNDATION_DLL xiiGALRenderCommandEncoder : public xiiGALCommandEncoder
{
public:
  xiiGALRenderCommandEncoder(xiiGALDevice& device, xiiGALCommandEncoderRenderState& renderState, xiiGALCommandEncoderCommonPlatformInterface& commonImpl, xiiGALCommandEncoderRenderPlatformInterface& renderImpl);
  virtual ~xiiGALRenderCommandEncoder();

  // Draw functions

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, xiiUInt8 uiStencilClear = 0x0u);

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
  void SetVertexDeclaration(xiiGALVertexDeclarationHandle hVertexDeclaration);

  xiiGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_RenderState.m_Topology; }
  void                          SetPrimitiveTopology(xiiGALPrimitiveTopology::Enum Topology);

  void SetBlendState(xiiGALBlendStateHandle hBlendState, const xiiColor& BlendFactor = xiiColor::White, xiiUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState, xiiUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const xiiRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const xiiRectU32& rect);

  void SetStreamOutBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer, xiiUInt32 uiOffset);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  xiiUInt32 m_uiDrawCalls = 0;

  xiiGALCommandEncoderRenderState& m_RenderState;

  xiiGALCommandEncoderRenderPlatformInterface& m_RenderImpl;
};
