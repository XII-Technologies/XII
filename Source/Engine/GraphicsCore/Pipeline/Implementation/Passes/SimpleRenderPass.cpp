#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/SimpleRenderPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleRenderPass, 1, xiiRTTIDefaultAllocator<xiiSimpleRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinColour),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_MEMBER_PROPERTY("Message", m_sMessage),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimpleRenderPass::xiiSimpleRenderPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiSimpleRenderPass::~xiiSimpleRenderPass() = default;

xiiResult xiiSimpleRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sMessage;

  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sMessage;

  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinColour.m_uiInputIndex])
  {
    pOutputs[m_PinColour.m_uiOutputIndex] = *pInputs[m_PinColour.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No colour attachment input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  // Depth stencil attachment.
  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil attachment input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pOutputs);

  m_pRenderPass.Clear();
  m_FramebufferCache.Clear();

  auto pColourInput = pInputs[m_PinColour.m_uiInputIndex];
  if (pColourInput == nullptr)
    return XII_FAILURE;

  auto pDepthInput = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
    return XII_FAILURE;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderPassCreationDescription renderPassDescription;

  xiiGALRenderPassAttachmentDescription& depthAttachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
  depthAttachment.m_Format                               = pDepthInput->m_Resource.m_Texture.m_Description.m_Format;
  depthAttachment.m_uiSampleCount                        = pDepthInput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
  depthAttachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
  depthAttachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
  depthAttachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
  depthAttachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
  depthAttachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
  depthAttachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

  xiiGALRenderPassAttachmentDescription& colourAttachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
  colourAttachment.m_Format                               = pColourInput->m_Resource.m_Texture.m_Description.m_Format;
  colourAttachment.m_uiSampleCount                        = pColourInput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
  colourAttachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
  colourAttachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
  colourAttachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
  colourAttachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
  colourAttachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
  colourAttachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;

  xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses.ExpandAndGetRef();
  subpass.m_DepthStencilAttachment.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = 0U, .m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite});
  subpass.m_RenderTargetAttachments.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = 1U, .m_ResourceStateFlags = xiiGALResourceStateFlags::RenderTarget});

  xiiGALSubPassDependencyDescription& dependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
  dependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
  dependency.m_uiDestinationSubPass              = 0U;
  dependency.m_SourceStageFlags                  = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_DestinationStageFlags             = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_SourceAccessFlags                 = xiiGALAccessFlags::DepthStencilWrite | xiiGALAccessFlags::RenderTargetWrite;
  dependency.m_DestinationAccessFlags            = xiiGALAccessFlags::DepthStencilWrite | xiiGALAccessFlags::RenderTargetWrite;

  m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);

  return XII_SUCCESS;
}

void xiiSimpleRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pFrameConstants = pInputs[m_PinFrameConstants.m_uiInputIndex];
  if (pFrameConstants == nullptr)
    return;

  auto pColourAttachment = pInputs[m_PinColour.m_uiInputIndex];
  if (pColourAttachment == nullptr)
    return;

  auto pDepthStencil = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthStencil == nullptr)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiSharedPtr<xiiGALTextureView>             pDepthStencilView     = pDepthStencil->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil);
  xiiSharedPtr<xiiGALTextureView>             pColourAttachmentView = pColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  const xiiGALTextureCreationDescription&     textureDescription    = pDepthStencil->m_Resource.m_Texture.m_pTexture->GetDescription();
  const xiiGALTextureViewCreationDescription& viewDescription       = pDepthStencilView->GetDescription();

  // Create or retrieve a corresponding framebuffer from the cache if present.
  xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
  {
    xiiGALFramebufferCreationDescription frameBufferDescription;
    frameBufferDescription.m_pRenderPass       = m_pRenderPass;
    frameBufferDescription.m_FramebufferSize   = textureDescription.m_Size;
    frameBufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
    frameBufferDescription.m_Attachments.PushBack(pDepthStencilView);
    frameBufferDescription.m_Attachments.PushBack(pColourAttachmentView);

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

    // Simple Opaque
    {
      xiiGALScopedDebugGroup group(renderViewContext.m_pCommandList, "Render Simple Opaque Static Objects");

      RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleOpaque);
    }
  }
  renderViewContext.m_pCommandList->End();
}
