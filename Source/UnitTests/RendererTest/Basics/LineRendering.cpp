#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

xiiTestAppRun xiiRendererTestBasics::SubtestLineRendering()
{
  BeginFrame();

  xiiColor clear(0, 0, 0, 0);
  ClearScreen(clear);

  RenderLineObjects(xiiShaderBindFlags::Default);

  XII_TEST_IMAGE(0, 150);

  EndFrame();

  return m_iFrame < 0 ? xiiTestAppRun::Continue : xiiTestAppRun::Quit;
}
