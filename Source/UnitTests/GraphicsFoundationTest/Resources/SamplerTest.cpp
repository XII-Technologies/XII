/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>

XII_CREATE_SIMPLE_TEST(Resources, Sampler)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Creation, retention, and deduplication")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALSamplerCreationDescription description;
      description.m_MinFilter          = xiiGALFilterType::Point;
      description.m_MagFilter          = xiiGALFilterType::Linear;
      description.m_MipFilter          = xiiGALFilterType::Point;
      description.m_AddressU           = xiiGALTextureAddressMode::Wrap;
      description.m_AddressV           = xiiGALTextureAddressMode::Mirror;
      description.m_AddressW           = xiiGALTextureAddressMode::Clamp;
      description.m_fMipLODBias        = 0.5f;
      description.m_fMinLOD            = 1.0f;
      description.m_fMaxLOD            = 8.0f;

      xiiSharedPtr<xiiGALSampler> pSamplerA = environment.GetDevice()->CreateSampler(description);
      xiiSharedPtr<xiiGALSampler> pSamplerB = environment.GetDevice()->CreateSampler(description);
      XII_TEST_BOOL(pSamplerA != nullptr);
      XII_TEST_BOOL(pSamplerB != nullptr);
      if (pSamplerA != nullptr)
      {
        XII_TEST_BOOL(pSamplerA->GetDescription() == description);
        XII_TEST_BOOL(pSamplerA->GetDevice().Borrow() == environment.GetDevice());
        XII_TEST_BOOL(pSamplerA == pSamplerB);
      }
    }
  }
}
