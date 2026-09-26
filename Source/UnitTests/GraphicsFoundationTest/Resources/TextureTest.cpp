/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>

XII_CREATE_SIMPLE_TEST(Resources, Texture)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "2D texture and default views")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALTextureCreationDescription description;
      description.m_Type               = xiiGALResourceDimension::Texture2D;
      description.m_Size               = xiiSizeU32(32U, 16U);
      description.m_uiArraySizeOrDepth = 1U;
      description.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      description.m_uiMipLevels        = 3U;
      description.m_uiSampleCount      = 1U;
      description.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
      description.m_Usage              = xiiGALResourceUsage::Mutable;

      xiiSharedPtr<xiiGALTexture> pTexture = environment.GetDevice()->CreateTexture(description);
      XII_TEST_BOOL(pTexture != nullptr);
      if (pTexture == nullptr)
        continue;

      XII_TEST_BOOL(pTexture->GetDescription() == description);
      XII_TEST_BOOL(pTexture->GetDevice().Borrow() == environment.GetDevice());
      XII_TEST_BOOL(pTexture->GetMemoryConsumption() >= 32U * 16U * 4U);
      XII_TEST_BOOL(pTexture->GetExternalMemoryKind().IsNoFlagSet());

      xiiSharedPtr<xiiGALTextureView> pSRV = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      xiiSharedPtr<xiiGALTextureView> pRTV = pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
      XII_TEST_BOOL(pSRV != nullptr);
      XII_TEST_BOOL(pRTV != nullptr);
      XII_TEST_BOOL(pSRV->GetTexture() == pTexture);
      XII_TEST_BOOL(pRTV->GetTexture() == pTexture);
      XII_TEST_INT(pSRV->GetDescription().m_uiMipLevelCount, 3U);
      XII_TEST_INT(pRTV->GetDescription().m_uiMipLevelCount, 1U);
    }
  }
}
