/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

XII_CREATE_SIMPLE_TEST(Resources, Framebuffer)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compatible render target")
  {
    xiiGALRenderPassCreationDescription renderPassDescription;
    auto&                               attachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
    attachment.m_Format                            = xiiGALResourceFormat::RGBA8UNormalized;
    attachment.m_uiSampleCount                     = 1U;
    attachment.m_LoadOperation                     = xiiGALAttachmentLoadOperation::Clear;
    attachment.m_StoreOperation                    = xiiGALAttachmentStoreOperation::Store;
    attachment.m_InitialStateFlags                 = xiiGALResourceStateFlags::Undefined;
    attachment.m_FinalStateFlags                   = xiiGALResourceStateFlags::ShaderResource;
    renderPassDescription.m_SubPasses.ExpandAndGetRef().m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});
    xiiSharedPtr<xiiGALRenderPass> pRenderPass = environment.GetDevice()->CreateRenderPass(renderPassDescription);
    XII_TEST_BOOL(pRenderPass != nullptr);

    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type            = xiiGALResourceDimension::Texture2D;
    textureDescription.m_Size            = xiiSizeU32(64U, 32U);
    textureDescription.m_Format          = xiiGALResourceFormat::RGBA8UNormalized;
    textureDescription.m_BindFlags       = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    xiiSharedPtr<xiiGALTexture> pTexture = environment.GetDevice()->CreateTexture(textureDescription);
    XII_TEST_BOOL(pTexture != nullptr);

    xiiSharedPtr<xiiGALTextureView> pRenderTargetView = pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    XII_TEST_BOOL(pRenderTargetView != nullptr);

    xiiGALFramebufferCreationDescription description;
    description.m_pRenderPass = pRenderPass;
    description.m_Attachments.PushBack(pRenderTargetView);
    description.m_FramebufferSize   = xiiSizeU32(64U, 32U);
    description.m_uiArraySliceCount = 1U;

    xiiSharedPtr<xiiGALFramebuffer> pFramebuffer = environment.GetDevice()->CreateFramebuffer(description);
    XII_TEST_BOOL(pFramebuffer != nullptr);
    if (pFramebuffer != nullptr)
    {
      XII_TEST_BOOL(pFramebuffer->GetDescription() == description);
      XII_TEST_BOOL(pFramebuffer->GetDevice().Borrow() == environment.GetDevice());
    }
  }
}
