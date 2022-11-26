#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

xiiTestAppRun xiiRendererTestBasics::SubtestBlendStates()
{
  BeginFrame();

  xiiGALBlendStateHandle hState;

  xiiGALBlendStateCreationDescription StateDesc;
  StateDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = true;

  if (m_iFrame == 0)
  {
    // StateDesc.m_RenderTargetBlendDescriptions[0].
  }

  if (m_iFrame == 1)
  {
    StateDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend = xiiGALBlend::SrcAlpha;
    StateDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend   = xiiGALBlend::InvSrcAlpha;
  }

  xiiColor clear(0, 0, 0, 0);
  // if (StateDesc.m_bDepthClip)
  //  clear.r = 0.5f;
  // if (StateDesc.m_bFrontCounterClockwise)
  //  clear.g = 0.5f;
  // if (StateDesc.m_CullMode == xiiGALCullMode::Front)
  //  clear.b = 0.5f;
  // if (StateDesc.m_CullMode == xiiGALCullMode::Back)
  //  clear.b = 1.0f;

  ClearScreen(clear);

  hState = m_pDevice->CreateBlendState(StateDesc);
  XII_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create blend state!");

  xiiRenderContext::GetDefaultInstance()->GetRenderCommandEncoder()->SetBlendState(hState);

  RenderObjects(xiiShaderBindFlags::NoBlendState);

  XII_TEST_IMAGE(m_iFrame, 150);

  EndFrame();

  m_pDevice->DestroyBlendState(hState);

  return m_iFrame < 1 ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}
