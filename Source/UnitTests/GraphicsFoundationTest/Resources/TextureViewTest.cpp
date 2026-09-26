/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>

XII_CREATE_SIMPLE_TEST(Resources, TextureView)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Array and mip subrange view")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALTextureCreationDescription textureDescription;
      textureDescription.m_Type               = xiiGALResourceDimension::Texture2DArray;
      textureDescription.m_Size               = xiiSizeU32(16U, 16U);
      textureDescription.m_uiArraySizeOrDepth = 4U;
      textureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      textureDescription.m_uiMipLevels        = 4U;
      textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
      textureDescription.m_Usage              = xiiGALResourceUsage::Mutable;
      xiiSharedPtr<xiiGALTexture> pTexture = environment.GetDevice()->CreateTexture(textureDescription);
      XII_TEST_BOOL(pTexture != nullptr);
      if (pTexture == nullptr)
        continue;

      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType                  = xiiGALTextureViewType::ShaderResource;
      viewDescription.m_ResourceDimension         = xiiGALResourceDimension::Texture2DArray;
      viewDescription.m_uiMostDetailedMip         = 1U;
      viewDescription.m_uiMipLevelCount           = 2U;
      viewDescription.m_uiFirstArrayOrDepthSlice  = 1U;
      viewDescription.m_uiArrayOrDepthSlicesCount = 2U;
      viewDescription.m_ComponentSwizzle.m_R      = xiiGALTextureComponentSwizzle::B;
      viewDescription.m_ComponentSwizzle.m_B      = xiiGALTextureComponentSwizzle::R;

      xiiSharedPtr<xiiGALTextureView> pView = pTexture->CreateView(viewDescription);
      XII_TEST_BOOL(pView != nullptr);
      if (pView != nullptr)
      {
        XII_TEST_BOOL(pView->GetTexture() == pTexture);
        XII_TEST_BOOL(pView->GetDescription() == viewDescription);
        XII_TEST_BOOL(pView->GetDescription().m_Format == xiiGALResourceFormat::RGBA8UNormalized);
        XII_TEST_BOOL(pView->GetDevice().Borrow() == environment.GetDevice());
      }
    }
  }
}
