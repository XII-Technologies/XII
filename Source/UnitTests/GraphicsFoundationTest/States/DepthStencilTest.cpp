/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

XII_CREATE_SIMPLE_TEST(States, DepthStencilState)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Defaults and mutation sensitivity")
  {
    xiiGALDepthStencilStateCreationDescription a;
    xiiGALDepthStencilStateCreationDescription b;
    XII_TEST_BOOL(a == b);
    XII_TEST_BOOL(a.m_bDepthEnable);
    XII_TEST_BOOL(a.m_bDepthWriteEnable);
    XII_TEST_BOOL(a.m_ComparisonDepthFunction == xiiGALComparisonFunction::Less);
    XII_TEST_BOOL(!a.m_bStencilEnable);

    b.m_bStencilEnable                       = true;
    b.m_uiStencilReadMask                    = 0x0FU;
    b.m_FrontFace.m_StencilPassOperation     = xiiGALStencilOperation::Replace;
    b.m_BackFace.m_ComparisonFunction        = xiiGALComparisonFunction::GreaterEqual;
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

      xiiGALDepthStencilStateCreationDescription description;
      description.m_bDepthWriteEnable                = false;
      description.m_ComparisonDepthFunction          = xiiGALComparisonFunction::GreaterEqual;
      description.m_bStencilEnable                   = true;
      description.m_FrontFace.m_StencilPassOperation = xiiGALStencilOperation::Replace;

      xiiSharedPtr<xiiGALDepthStencilState> pState = environment.GetDevice()->CreateDepthStencilState(description);
      XII_TEST_BOOL(pState != nullptr);
      if (pState != nullptr)
      {
        XII_TEST_BOOL(pState->GetDescription() == description);
        XII_TEST_BOOL(pState->GetDevice().Borrow() == environment.GetDevice());
      }
    }
  }
}
