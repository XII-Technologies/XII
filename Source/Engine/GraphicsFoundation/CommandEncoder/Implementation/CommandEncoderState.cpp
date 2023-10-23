#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>

void xiiGALCommandEncoderState::InvalidateState()
{
  m_hShader = xiiGALShaderHandle();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hConstantBuffers); ++i)
  {
    m_hConstantBuffers[i].Invalidate();
  }

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    m_hBufferViews[i].Clear();
    m_pResourcesForBufferViews[i].Clear();

    m_hTextureViews[i].Clear();
    m_pResourcesForTextureViews[i].Clear();
  }

  m_hUnorderedAccessBufferViews.Clear();
  m_pResourcesForUnorderedAccessBufferViews.Clear();

  m_hUnorderedAccessTextureViews.Clear();
  m_pResourcesForUnorderedAccessTextureViews.Clear();

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    for (xiiUInt32 j = 0; j < XII_GAL_MAX_SAMPLER_COUNT; ++j)
    {
      m_hSamplers[i][j].Invalidate();
    }
  }
}

void xiiGALCommandEncoderGraphicsState::InvalidateState()
{
  xiiGALCommandEncoderState::InvalidateState();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }
  m_hIndexBuffer.Invalidate();

  m_hInputLayout.Invalidate();
  m_Topology = xiiGALPrimitiveTopology::Undefined;

  m_hBlendState.Invalidate();
  m_BlendFactor  = xiiColor(0, 0, 0, 0);
  m_uiSampleMask = 0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect       = xiiRectU32(0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U);
  m_ViewPortRect      = xiiRectFloat(xiiMath::MaxValue<float>(), xiiMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = xiiMath::MaxValue<float>();
  m_fViewPortMaxDepth = -xiiMath::MaxValue<float>();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandEncoderState);
