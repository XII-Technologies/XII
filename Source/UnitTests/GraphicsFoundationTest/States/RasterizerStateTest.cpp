/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/States/RasterizerState.h>

XII_CREATE_SIMPLE_TEST(States, RasterizerState)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Defaults and mutation sensitivity")
  {
    xiiGALRasterizerStateCreationDescription a;
    xiiGALRasterizerStateCreationDescription b;
    XII_TEST_BOOL(a == b);
    XII_TEST_BOOL(a.m_FillMode == xiiGALFillMode::Solid);
    XII_TEST_BOOL(a.m_CullMode == xiiGALCullMode::Back);
    XII_TEST_BOOL(a.m_bAntialiasedLineEnable);

    b.m_CullMode                = xiiGALCullMode::Front;
    b.m_bFrontCounterClockwise = true;
    b.m_iDepthBias              = 7;
    b.m_fSlopeScaledDepthBias   = 1.5f;
    XII_TEST_BOOL(a != b);
    XII_TEST_BOOL(a.CalculateHash() != b.CalculateHash());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Device creation and retained description")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALRasterizerStateCreationDescription description;
      description.m_CullMode                = xiiGALCullMode::None;
      description.m_bFrontCounterClockwise = true;
      description.m_bDepthClipEnable       = true;
      description.m_bScissorEnable         = true;

      xiiSharedPtr<xiiGALRasterizerState> pState = environment.GetDevice()->CreateRasterizerState(description);
      XII_TEST_BOOL(pState != nullptr);
      if (pState != nullptr)
      {
        XII_TEST_BOOL(pState->GetDescription() == description);
        XII_TEST_BOOL(pState->GetDevice().Borrow() == environment.GetDevice());
      }
    }
  }
}
