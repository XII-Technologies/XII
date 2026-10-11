/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>

XII_CREATE_SIMPLE_TEST(Resources, Sampler)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());
  const auto initialDeviceReferences = environment.GetDevice()->GetRefCount();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Creation, retention, and deduplication")
  {
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter   = xiiGALFilterType::Point;
    description.m_MagFilter   = xiiGALFilterType::Linear;
    description.m_MipFilter   = xiiGALFilterType::Point;
    description.m_AddressU    = xiiGALTextureAddressMode::Wrap;
    description.m_AddressV    = xiiGALTextureAddressMode::Mirror;
    description.m_AddressW    = xiiGALTextureAddressMode::Clamp;
    description.m_fMipLODBias = 0.5f;
    description.m_fMinLOD     = 1.0f;
    description.m_fMaxLOD     = 8.0f;

    xiiSharedPtr<xiiGALSampler> pSamplerA = environment.GetDevice()->CreateSampler(description);
    xiiSharedPtr<xiiGALSampler> pSamplerB = environment.GetDevice()->CreateSampler(description);
    XII_TEST_BOOL(pSamplerA != nullptr);
    XII_TEST_BOOL(pSamplerB != nullptr);
    if (pSamplerA != nullptr)
    {
      XII_TEST_BOOL(pSamplerA->GetDescription() == description);
      XII_TEST_BOOL(pSamplerA->GetDevice().Borrow() == environment.GetDevice());
      XII_TEST_BOOL(pSamplerA == pSamplerB);
      pSamplerA.Clear();
      XII_TEST_INT(pSamplerB->GetRefCount(), 1);
      pSamplerB.Clear();
      pSamplerA = environment.GetDevice()->CreateSampler(description);
      XII_TEST_BOOL(pSamplerA != nullptr);
      XII_TEST_INT(pSamplerA->GetRefCount(), 1);
    }
  }
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(environment.GetDevice()->GetRefCount(), initialDeviceReferences);
}
