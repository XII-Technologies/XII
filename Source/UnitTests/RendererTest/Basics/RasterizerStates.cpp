#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

xiiTestAppRun xiiRendererTestBasics::SubtestRasterizerStates()
{
  BeginFrame();

  xiiGALRasterizerStateHandle hState;

  xiiGALRasterizerStateCreationDescription RasterStateDesc;

  if (m_iFrame == 0)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::None;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 1)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 2)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Front;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 3)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 4)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Front;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 5)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 6)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = false;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = true;
  }

  if (m_iFrame == 7)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = true;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::None;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 8)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = true;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 9)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame             = true;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Front;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 10)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame             = true;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Back;
    RasterStateDesc.m_bScissorTest           = false;
  }

  if (m_iFrame == 11)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame             = true;
    RasterStateDesc.m_CullMode               = xiiGALCullMode::Front;
    RasterStateDesc.m_bScissorTest           = false;
  }

  xiiColor clear(0, 0, 0, 0);
  if (!RasterStateDesc.m_bFrontCounterClockwise)
    clear.g = 0.5f;
  if (RasterStateDesc.m_CullMode == xiiGALCullMode::Front)
    clear.b = 0.5f;
  if (RasterStateDesc.m_CullMode == xiiGALCullMode::Back)
    clear.b = 1.0f;

  ClearScreen(clear);

  hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
  XII_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create rasterizer state!");

  xiiRenderContext::GetDefaultInstance()->GetRenderCommandEncoder()->SetRasterizerState(hState);

  xiiRenderContext::GetDefaultInstance()->GetRenderCommandEncoder()->SetScissorRect(xiiRectU32(100, 50, GetResolution().width / 2, GetResolution().height / 2));

  RenderObjects(xiiShaderBindFlags::NoRasterizerState);

  XII_TEST_IMAGE(m_iFrame, 150);

  EndFrame();

  m_pDevice->DestroyRasterizerState(hState);

  return m_iFrame < 11 ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}
