/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

XII_CREATE_SIMPLE_TEST(States, BlendState)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Descriptor equality and hashing")
  {
    xiiGALBlendStateCreationDescription a;
    xiiGALBlendStateCreationDescription b;
    a.m_RenderTargets.ExpandAndGetRef();
    b.m_RenderTargets.ExpandAndGetRef();

    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(a), xiiGALDescriptorHash::Hash(b));

    b.m_RenderTargets[0].m_bBlendEnable     = true;
    b.m_RenderTargets[0].m_SourceBlend      = xiiGALBlendFactor::SourceAlpha;
    b.m_RenderTargets[0].m_DestinationBlend = xiiGALBlendFactor::InverseSourceAlpha;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    a                                     = b;
    b.m_RenderTargets[0].m_BlendOperation = xiiGALBlendOperation::ReverseSubtract;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Device creation and retained description")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBlendStateCreationDescription description;
      auto&                               renderTarget = description.m_RenderTargets.ExpandAndGetRef();
      renderTarget.m_bBlendEnable                      = true;
      renderTarget.m_SourceBlend                       = xiiGALBlendFactor::SourceAlpha;
      renderTarget.m_DestinationBlend                  = xiiGALBlendFactor::InverseSourceAlpha;
      renderTarget.m_SourceBlendAlpha                  = xiiGALBlendFactor::One;
      renderTarget.m_DestinationBlendAlpha             = xiiGALBlendFactor::Zero;

      xiiSharedPtr<xiiGALBlendState> pState = environment.GetDevice()->CreateBlendState(description);
      XII_TEST_BOOL(pState != nullptr);
      if (pState != nullptr)
      {
        XII_TEST_BOOL(pState->GetDescription() == description);
        XII_TEST_BOOL(pState->GetDevice().Borrow() == environment.GetDevice());
        pState->SetDebugName("Alpha Blend State");
        XII_TEST_STRING(pState->GetDebugName(), "Alpha Blend State");
      }
    }
  }
}
