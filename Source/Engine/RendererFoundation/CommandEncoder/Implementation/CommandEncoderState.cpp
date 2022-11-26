#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void xiiGALCommandEncoderState::InvalidateState()
{
  m_hShader = xiiGALShaderHandle();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hConstantBuffers); ++i)
  {
    m_hConstantBuffers[i].Invalidate();
  }

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    m_hResourceViews[i].Clear();
    m_pResourcesForResourceViews[i].Clear();
  }

  m_hUnorderedAccessViews.Clear();
  m_pResourcesForUnorderedAccessViews.Clear();

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    for (xiiUInt32 j = 0; j < XII_GAL_MAX_SAMPLER_COUNT; j++)
    {
      m_hSamplerStates[i][j].Invalidate();
    }
  }
}

void xiiGALCommandEncoderRenderState::InvalidateState()
{
  xiiGALCommandEncoderState::InvalidateState();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }
  m_hIndexBuffer.Invalidate();

  m_hVertexDeclaration.Invalidate();
  m_Topology = xiiGALPrimitiveTopology::ENUM_COUNT;

  m_hBlendState.Invalidate();
  m_BlendFactor  = xiiColor(0, 0, 0, 0);
  m_uiSampleMask = 0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect       = xiiRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect      = xiiRectFloat(xiiMath::MaxValue<float>(), xiiMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = xiiMath::MaxValue<float>();
  m_fViewPortMaxDepth = -xiiMath::MaxValue<float>();
}
