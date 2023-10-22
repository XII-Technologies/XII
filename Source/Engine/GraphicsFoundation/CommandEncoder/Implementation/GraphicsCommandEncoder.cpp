#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Shader/InputLayout.h>

xiiGALGraphicsCommandEncoder::xiiGALGraphicsCommandEncoder(xiiGALDevice& ref_device, xiiGALCommandEncoderGraphicsState& ref_graphicsState, xiiGALCommandEncoderCommonPlatformInterface& ref_commonImpl, xiiGALCommandEncoderGraphicsPlatformInterface& ref_graphicsImpl) :
  xiiGALCommandEncoder(ref_device, ref_graphicsState, ref_commonImpl), m_GraphicsState(ref_graphicsState), m_GraphicsImpl(ref_graphicsImpl)
{
}

xiiGALGraphicsCommandEncoder::~xiiGALGraphicsCommandEncoder() = default;

void xiiGALGraphicsCommandEncoder::Clear(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  AssertRenderingThread();

  m_GraphicsImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void xiiGALGraphicsCommandEncoder::Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_GraphicsImpl.DrawPlatform(uiVertexCount, uiStartVertex);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  AssertRenderingThread();

  m_GraphicsImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  AssertRenderingThread();

  /// \todo Assert for instanced rendering

  m_GraphicsImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();

  /// \todo Assert for instanced rendering
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const xiiGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  XII_ASSERT_DEV(pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The buffer must be created with the xiiGALBindFlags::IndirectDrawArguments bind flag.");

  m_GraphicsImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo Assert for instanced rendering

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_GraphicsImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();

  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const xiiGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  XII_ASSERT_DEV(pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The buffer must be created with the xiiGALBindFlags::IndirectDrawArguments bind flag.");

  m_GraphicsImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void xiiGALGraphicsCommandEncoder::BeginStreamOut()
{
  AssertRenderingThread();

  /// \todo Assert for streamout support

  m_GraphicsImpl.BeginStreamOutPlatform();
}

void xiiGALGraphicsCommandEncoder::EndStreamOut()
{
  AssertRenderingThread();

  m_GraphicsImpl.EndStreamOutPlatform();
}

void xiiGALGraphicsCommandEncoder::SetIndexBuffer(xiiGALBufferHandle hIndexBuffer)
{
  if (m_GraphicsState.m_hIndexBuffer == hIndexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid index buffer handle!");

  XII_ASSERT_DEV(pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::IndexBuffer), "The buffer must be created with the xiiGALBindFlags::IndexBuffer bind flag.");

  m_GraphicsImpl.SetIndexBufferPlatform(pBuffer);

  m_GraphicsState.m_hIndexBuffer = hIndexBuffer;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetVertexBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hVertexBuffer)
{
  if (m_GraphicsState.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid vertex buffer handle!");

  XII_ASSERT_DEV(pBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::VertexBuffer), "The buffer must be created with the xiiGALBindFlags::VertexBuffer bind flag.");

  m_GraphicsImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_GraphicsState.m_hVertexBuffers[uiSlot] = hVertexBuffer;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetInputLayout(xiiGALInputLayoutHandle hInputLayout)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_hInputLayout == hInputLayout)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALInputLayout* pInputLayout = GetDevice().GetInputLayout(hInputLayout);
  XII_ASSERT_DEV(pInputLayout != nullptr, "Invalid input layout handle!");

  // Assert on vertex buffer type (if non-zero)

  m_GraphicsImpl.SetInputLayoutPlatform(pInputLayout);

  m_GraphicsState.m_hInputLayout = hInputLayout;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> topology)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_Topology == topology)
  {
    CountRedundantStateChange();
    return;
  }

  m_GraphicsImpl.SetPrimitiveTopologyPlatform(topology);

  m_GraphicsState.m_Topology = topology;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetBlendState(xiiGALBlendStateHandle hBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_hBlendState == hBlendState && m_GraphicsState.m_BlendFactor.IsEqualRGBA(blendFactor, 0.001f) && m_GraphicsState.m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);
  XII_ASSERT_DEV(pBlendState != nullptr, "Invalid blend state handle!");

  m_GraphicsImpl.SetBlendStatePlatform(pBlendState, blendFactor, uiSampleMask);

  m_GraphicsState.m_hBlendState  = hBlendState;
  m_GraphicsState.m_BlendFactor  = blendFactor;
  m_GraphicsState.m_uiSampleMask = uiSampleMask;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_hDepthStencilState == hDepthStencilState && m_GraphicsState.m_uiStencilRefValue == uiStencilRefValue)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);
  XII_ASSERT_DEV(pDepthStencilState != nullptr, "Invalid depth-stencil state handle!");

  m_GraphicsImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_GraphicsState.m_hDepthStencilState = hDepthStencilState;
  m_GraphicsState.m_uiStencilRefValue  = uiStencilRefValue;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_hRasterizerState == hRasterizerState)
  {
    CountRedundantStateChange();
    return;
  }

  const xiiGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);
  XII_ASSERT_DEV(pRasterizerState != nullptr, "Invalid rasterizer state handle!");

  m_GraphicsImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_GraphicsState.m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetViewport(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_ViewPortRect == rect && m_GraphicsState.m_fViewPortMinDepth == fMinDepth && m_GraphicsState.m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  m_GraphicsImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_GraphicsState.m_ViewPortRect      = rect;
  m_GraphicsState.m_fViewPortMinDepth = fMinDepth;
  m_GraphicsState.m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetScissorRect(const xiiRectU32& rect)
{
  AssertRenderingThread();

  if (m_GraphicsState.m_ScissorRect == rect)
  {
    CountRedundantStateChange();
    return;
  }

  m_GraphicsImpl.SetScissorRectPlatform(rect);

  m_GraphicsState.m_ScissorRect = rect;

  CountStateChange();
}

void xiiGALGraphicsCommandEncoder::SetStreamOutBuffer(xiiUInt32 uiSlot, xiiGALBufferHandle hBuffer, xiiUInt32 uiOffset)
{
  XII_ASSERT_NOT_IMPLEMENTED;

  CountStateChange();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_GraphicsCommandEncoder);
