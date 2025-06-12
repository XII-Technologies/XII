#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DepthOnlyPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthOnlyPass, 1, xiiRTTIDefaultAllocator<xiiDepthOnlyPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDepthOnlyPass::xiiDepthOnlyPass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
}

xiiDepthOnlyPass::~xiiDepthOnlyPass()
{
  m_pFramebuffer.Clear();
  m_pRenderPass.Clear();
}

bool xiiDepthOnlyPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs)
{
  // DepthStencil
  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiDepthOnlyPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Create render pass.
  if (auto pInput = pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    const auto& textureDescription = pInput->m_pTexture->GetDescription();

    xiiGALRenderPassCreationDescription renderPassDescription;
    auto&                               subpassDescription    = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    auto&                               dependencyDescription = renderPassDescription.m_Dependencies.ExpandAndGetRef();
    auto&                               attachmentDescription = renderPassDescription.m_Attachments.ExpandAndGetRef();

    dependencyDescription.m_uiSourceSubPass       = XII_GAL_SUBPASS_EXTERNAL;
    dependencyDescription.m_uiDestinationSubPass  = 0U;
    dependencyDescription.m_SourceStageFlags      = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
    dependencyDescription.m_DestinationStageFlags = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;

    attachmentDescription.m_Format                = textureDescription.m_Format;
    attachmentDescription.m_uiSampleCount         = textureDescription.m_uiSampleCount;
    attachmentDescription.m_LoadOperation         = xiiGALAttachmentLoadOperation::Load;
    attachmentDescription.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
    attachmentDescription.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;
    attachmentDescription.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
    attachmentDescription.m_InitialStateFlags     = xiiGALResourceStateFlags::DepthWrite;
    attachmentDescription.m_FinalStateFlags       = xiiGALResourceStateFlags::DepthWrite;

    auto& depthAttachmentReference                = subpassDescription.m_DepthStencilAttachment.ExpandAndGetRef();
    depthAttachmentReference.m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite;
    depthAttachmentReference.m_uiAttachmentIndex  = 0U;

    dependencyDescription.m_SourceAccessFlags      = xiiGALAccessFlags::DepthStencilWrite;
    dependencyDescription.m_DestinationAccessFlags = xiiGALAccessFlags::DepthStencilWrite;

    m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);
    XII_ASSERT_DEV(m_pRenderPass != nullptr, "Failed to create render pass.");
  }
}

void xiiDepthOnlyPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pInput = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pInput == nullptr)
    return;

  if (!m_pRenderPass)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup Framebuffer.
  {
    if (m_pFramebuffer)
    {
      const auto& pDepthStencilView = pInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil);

      if (m_pFramebuffer->GetDescription().m_Attachments.PeekBack() != pDepthStencilView)
      {
        m_pFramebuffer.Clear();
      }
    }

    if (!m_pFramebuffer)
    {
      const auto& attachmentDescription     = pInput->m_pTexture->GetDescription();
      const auto& pDepthStencilView         = pInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil);
      const auto& attachmentViewDescription = pDepthStencilView->GetDescription();
      xiiVec3U32  vSize                     = xiiGALTextureUtilities::GetMipLevelSize(attachmentViewDescription.m_uiMostDetailedMip, attachmentDescription);

      xiiGALFramebufferCreationDescription framebufferDescription;
      framebufferDescription.m_pRenderPass       = m_pRenderPass;
      framebufferDescription.m_FramebufferSize   = {vSize.x, vSize.y};
      framebufferDescription.m_uiArraySliceCount = attachmentDescription.GetArraySize();
      framebufferDescription.m_Attachments.PushBack(pDepthStencilView);

      m_pFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
      XII_ASSERT_DEV(m_pFramebuffer != nullptr, "Failed to create frame buffer.");
    }
  }

  xiiGALBeginRenderPassDescription renderPassDescription(m_pRenderPass, m_pFramebuffer);

  auto& clearValue                      = renderPassDescription.m_ClearValues.ExpandAndGetRef();
  clearValue.m_ResourceFormat           = pInput->m_TextureDescription.m_Format;
  clearValue.m_DepthStencil.m_fDepth    = 1.0f;
  clearValue.m_DepthStencil.m_uiStencil = 0U;

  if (auto pCommandList = pDevice->GetDefaultCommandQueue()->BeginCommandList())
  {
    pCommandList->BeginDebugGroup(GetName());
    {
      renderViewContext.SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
      renderViewContext.SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

      if (m_pFramebuffer->GetDescription().m_Attachments.PeekBack()->GetTexture()->GetDescription().m_uiSampleCount > 1)
      {
        renderViewContext.SetShaderPermutationVariable("MSAA", "TRUE");
      }
      else
      {
        renderViewContext.SetShaderPermutationVariable("MSAA", "FALSE");
      }

      RenderDataWithCategory(renderViewContext, pCommandList, xiiDefaultRenderDataCategories::LitOpaque);
      RenderDataWithCategory(renderViewContext, pCommandList, xiiDefaultRenderDataCategories::LitMasked);
    }
    pCommandList->EndDebugGroup();
    pCommandList->Submit();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_DepthOnlyPass);
