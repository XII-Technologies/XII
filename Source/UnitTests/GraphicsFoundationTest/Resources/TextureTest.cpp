/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>

namespace
{
  class DefaultTextureViewThread : public xiiThread
  {
  public:
    explicit DefaultTextureViewThread(xiiSharedPtr<xiiGALTexture> texture) : xiiThread("Default texture view lifetime"), m_Texture(std::move(texture)) {}
    bool m_bFailed = false;

  private:
    xiiUInt32 Run() override
    {
      for (xiiUInt32 i = 0; i < 1024; ++i)
      {
        auto view = m_Texture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
        if (view == nullptr || view != m_Texture->GetDefaultView(xiiGALTextureViewType::ShaderResource) || view->GetTexture() != m_Texture)
          m_bFailed = true;
      }
      return 0;
    }
    xiiSharedPtr<xiiGALTexture> m_Texture;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Resources, Texture)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());
  const auto initialDeviceReferences = environment.GetDevice()->GetRefCount();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "2D texture and default views")
  {
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
    XII_TEST_INT(pTexture->GetRefCount(), 1); // No default view owns the resource before it is requested.

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
    XII_TEST_BOOL(pSRV == pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    XII_TEST_INT(pSRV->GetRefCount(), 1); // The cache itself owns no reference.
    pSRV.Clear();
    XII_TEST_INT(pTexture->GetRefCount(), 2); // The client and the retained render-target view.
    pSRV = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    XII_TEST_BOOL(pSRV != nullptr);
    pTexture.Clear();
    XII_TEST_INT(pSRV->GetTexture()->GetDescription().m_Size.width, 32);
    XII_TEST_BOOL(pSRV->GetTexture() == pRTV->GetTexture());
    pRTV.Clear();
    XII_TEST_INT(pSRV->GetTexture()->GetRefCount(), 2); // The view plus this temporary query.
    pSRV.Clear();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Concurrent default view acquisition and last release")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {4, 4};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource;
    auto texture            = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(texture != nullptr))
      return;
    // On devices without concurrent creation, retain the initial view so both
    // workers exercise cache acquisition without creating platform objects.
    auto                     retained = environment.GetDevice()->GetFeatures().m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled ?
      xiiSharedPtr<xiiGALTextureView>() :
      texture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    DefaultTextureViewThread first(texture), second(texture);
    first.Start();
    second.Start();
    first.Join();
    second.Join();
    XII_TEST_BOOL(!first.m_bFailed && !second.m_bFailed);
  }
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(environment.GetDevice()->GetRefCount(), initialDeviceReferences);
}
