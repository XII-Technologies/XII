/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

XII_CREATE_SIMPLE_TEST(Resources, RenderPass)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Single color attachment")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALRenderPassCreationDescription description;
      auto& attachment                 = description.m_Attachments.ExpandAndGetRef();
      attachment.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      attachment.m_uiSampleCount      = 1U;
      attachment.m_LoadOperation      = xiiGALAttachmentLoadOperation::Clear;
      attachment.m_StoreOperation     = xiiGALAttachmentStoreOperation::Store;
      attachment.m_InitialStateFlags  = xiiGALResourceStateFlags::Undefined;
      attachment.m_FinalStateFlags    = xiiGALResourceStateFlags::ShaderResource;

      auto& subPass = description.m_SubPasses.ExpandAndGetRef();
      subPass.m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});

      xiiSharedPtr<xiiGALRenderPass> pRenderPass = environment.GetDevice()->CreateRenderPass(description);
      XII_TEST_BOOL(pRenderPass != nullptr);
      if (pRenderPass != nullptr)
      {
        XII_TEST_BOOL(pRenderPass->GetDescription() == description);
        XII_TEST_BOOL(pRenderPass->GetDevice().Borrow() == environment.GetDevice());
        pRenderPass->SetDebugName("Unit Test Render Pass");
        XII_TEST_STRING(pRenderPass->GetDebugName(), "Unit Test Render Pass");
      }
    }
  }
}
