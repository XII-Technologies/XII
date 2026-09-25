/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

XII_CREATE_SIMPLE_TEST(Utilities, DescriptorHash)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Framebuffer dimensions participate in the hash")
  {
    xiiGALFramebufferCreationDescription a;
    a.m_FramebufferSize   = xiiSizeU32(1280U, 720U);
    a.m_uiArraySliceCount = 1U;
    xiiGALFramebufferCreationDescription b = a;

    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(a), xiiGALDescriptorHash::Hash(b));

    b.m_FramebufferSize.width = 1920U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b                           = a;
    b.m_FramebufferSize.height = 1080U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Render-pass subpass contents participate in the hash")
  {
    xiiGALRenderPassCreationDescription a;
    auto& subpass = a.m_SubPasses.ExpandAndGetRef();
    subpass.m_RenderTargetAttachments.PushBack(xiiGALAttachmentReferenceDescription(0U, xiiGALResourceStateFlags::RenderTarget));
    xiiGALRenderPassCreationDescription b = a;

    XII_TEST_BOOL(xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_INT(xiiGALDescriptorHash::Hash(a), xiiGALDescriptorHash::Hash(b));

    b.m_SubPasses[0].m_RenderTargetAttachments[0].m_uiAttachmentIndex = 1U;
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));

    b = a;
    b.m_SubPasses[0].m_PreserveAttachments.PushBack(3U);
    XII_TEST_BOOL(!xiiGALDescriptorHash::Equal(a, b));
    XII_TEST_BOOL(xiiGALDescriptorHash::Hash(a) != xiiGALDescriptorHash::Hash(b));
  }
}
