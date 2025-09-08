#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DepthOnlyPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthOnlyPass, 3, xiiRTTIDefaultAllocator<xiiDepthOnlyPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_MEMBER_PROPERTY("RenderStaticObjects", m_bRenderStaticObjects)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RenderDynamicObjects", m_bRenderDynamicObjects)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RenderTransparentObjects", m_bRenderTransparentObjects)->AddAttributes(new xiiDefaultValueAttribute(false)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDepthOnlyPass::xiiDepthOnlyPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiDepthOnlyPass::~xiiDepthOnlyPass() = default;

xiiResult xiiDepthOnlyPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_bRenderStaticObjects;
  inout_stream << m_bRenderDynamicObjects;
  inout_stream << m_bRenderTransparentObjects;

  return XII_SUCCESS;
}

xiiResult xiiDepthOnlyPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_bRenderStaticObjects;
  inout_stream >> m_bRenderDynamicObjects;
  inout_stream >> m_bRenderTransparentObjects;

  return XII_SUCCESS;
}

xiiResult xiiDepthOnlyPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiDepthOnlyPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pOutputs);

  m_pRenderPass.Clear();
  m_FramebufferCache.Clear();

  auto pInput = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pInput == nullptr)
    return XII_FAILURE;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderPassCreationDescription renderPassDescription;

  xiiGALRenderPassAttachmentDescription& attachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
  attachment.m_Format                               = pInput->m_Resource.m_Texture.m_Description.m_Format;
  attachment.m_uiSampleCount                        = pInput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
  attachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
  attachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
  attachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
  attachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
  attachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
  attachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

  xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses.ExpandAndGetRef();
  subpass.m_DepthStencilAttachment.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = 0U, .m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite});

  xiiGALSubPassDependencyDescription& dependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
  dependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
  dependency.m_uiDestinationSubPass              = 0U;
  dependency.m_SourceStageFlags                  = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_DestinationStageFlags             = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_SourceAccessFlags                 = xiiGALAccessFlags::DepthStencilRead | xiiGALAccessFlags::DepthStencilWrite;
  dependency.m_DestinationAccessFlags            = xiiGALAccessFlags::DepthStencilRead | xiiGALAccessFlags::DepthStencilWrite;

  m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);

  return XII_SUCCESS;
}

void xiiDepthOnlyPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pFrameConstants = pInputs[m_PinFrameConstants.m_uiInputIndex];
  if (pFrameConstants == nullptr)
    return;

  auto pDepthStencil = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthStencil == nullptr)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiSharedPtr<xiiGALTextureView>             pDepthStencilView  = pDepthStencil->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil);
  const xiiGALTextureCreationDescription&     textureDescription = pDepthStencil->m_Resource.m_Texture.m_pTexture->GetDescription();
  const xiiGALTextureViewCreationDescription& viewDescription    = pDepthStencilView->GetDescription();

  // Create or retrieve a corresponding framebuffer from the cache if present.
  xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
  {
    xiiGALFramebufferCreationDescription frameBufferDescription;
    frameBufferDescription.m_pRenderPass       = m_pRenderPass;
    frameBufferDescription.m_FramebufferSize   = textureDescription.m_Size;
    frameBufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
    frameBufferDescription.m_Attachments.PushBack(pDepthStencilView);

    for (xiiSharedPtr<xiiGALFramebuffer>& pCachedFramebuffer : m_FramebufferCache)
    {
      if (pCachedFramebuffer->GetDescription() == frameBufferDescription)
      {
        pFramebuffer = pCachedFramebuffer;
        break;
      }
    }

    if (!pFramebuffer)
    {
      pFramebuffer = pDevice->CreateFramebuffer(frameBufferDescription);

      m_FramebufferCache.PushBack(pFramebuffer);
    }
  }

  renderViewContext.m_CommandListData.m_pRenderPass      = m_pRenderPass;
  renderViewContext.m_CommandListData.m_pFramebuffer     = pFramebuffer;
  renderViewContext.m_CommandListData.m_pGlobalConstants = pFrameConstants->m_Resource.m_Buffer.m_pBuffer;

  renderViewContext.m_pCommandList->Begin();
  {
    xiiGALScopedDebugGroup scope(renderViewContext.m_pCommandList, GetName());

    // Opaque
    if (m_bRenderStaticObjects)
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Opaque Static Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaqueStatic);
    }
    if (m_bRenderDynamicObjects)
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Opaque Dynamic Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaqueDynamic);
    }

    // Masked
    if (m_bRenderStaticObjects)
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Masked Static Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMaskedStatic);
    }
    if (m_bRenderDynamicObjects)
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Masked Dynamic Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMaskedDynamic);
    }

    // Transparent
    if (m_bRenderTransparentObjects)
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Transparent Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitTransparent);
    }
  }
  renderViewContext.m_pCommandList->End();
}
