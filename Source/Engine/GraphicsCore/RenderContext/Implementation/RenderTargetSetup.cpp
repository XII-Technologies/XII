#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

bool xiiRenderTargets::operator==(const xiiRenderTargets& other) const
{
  if (m_pDSTarget != other.m_pDSTarget)
    return false;

  for (xiiUInt8 uiRTIndex = 0; uiRTIndex < XII_ARRAY_SIZE(m_pRTs); ++uiRTIndex)
  {
    if (m_pRTs[uiRTIndex] != other.m_pRTs[uiRTIndex])
      return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////

void xiiRenderingSetup::Build()
{
  m_RenderPassDescription.m_Attachments.Clear();
  m_FramebufferDescription.m_Attachments.Clear();
  m_ClearValues.Clear();

  for (xiiUInt32 i = 0; i < m_Attachments.GetCount(); ++i)
  {
    const Attachment&                           attachment         = m_Attachments[i];
    const xiiGALTextureViewCreationDescription& viewDescription    = attachment.m_pView->GetDescription();
    const xiiGALTextureCreationDescription&     textureDescription = attachment.m_pView->GetTexture()->GetDescription();

    xiiGALRenderPassAttachmentDescription renderPassAttachment;
    renderPassAttachment.m_Format         = viewDescription.m_Format;
    renderPassAttachment.m_uiSampleCount  = textureDescription.m_uiSampleCount;
    renderPassAttachment.m_LoadOperation  = attachment.m_LoadOp;
    renderPassAttachment.m_StoreOperation = attachment.m_StoreOp;

    if (xiiGALResourceFormat::IsDepthFormat(viewDescription.m_Format))
    {
      renderPassAttachment.m_StencilLoadOperation  = attachment.m_StencilLoadOp;
      renderPassAttachment.m_StencilStoreOperation = attachment.m_StencilStoreOp;

      renderPassAttachment.m_InitialStateFlags = xiiGALResourceStateFlags::DepthWrite;
      renderPassAttachment.m_FinalStateFlags   = xiiGALResourceStateFlags::DepthWrite;
    }
    else
    {
      renderPassAttachment.m_InitialStateFlags = xiiGALResourceStateFlags::RenderTarget;
      renderPassAttachment.m_FinalStateFlags   = xiiGALResourceStateFlags::RenderTarget;
    }

    m_RenderPassDescription.m_Attachments.PushBack(renderPassAttachment);
    m_FramebufferDescription.m_Attachments.PushBack(attachment.m_pView);

    if (renderPassAttachment.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
    {
      m_ClearValues.EnsureCount(i + 1);

      m_ClearValues[i] = attachment.m_ClearValue;
    }
  }

  // If no sub pass was defined externally, create one default sub pass.
  if (m_RenderPassDescription.m_SubPasses.IsEmpty())
  {
    xiiGALSubPassDescription& defaultSubPass = m_RenderPassDescription.m_SubPasses.ExpandAndGetRef();

    for (xiiUInt32 i = 0; i < m_Attachments.GetCount(); ++i)
    {
      const Attachment&                           attachment      = m_Attachments[i];
      const xiiGALTextureViewCreationDescription& viewDescription = attachment.m_pView->GetDescription();

      if (xiiGALResourceFormat::IsDepthFormat(viewDescription.m_Format))
      {
        defaultSubPass.m_DepthStencilAttachment.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = i, .m_ResourceStateFlags = attachment.m_SubpassState});
      }
      else
      {
        defaultSubPass.m_RenderTargetAttachments.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = i, .m_ResourceStateFlags = attachment.m_SubpassState});
      }
    }
  }

  // If no sub pass dependency was defined externally, create one default sub pass dependency.
  if (m_RenderPassDescription.m_Dependencies.IsEmpty())
  {
    xiiGALSubPassDependencyDescription& defaultSubPassDependency = m_RenderPassDescription.m_Dependencies.ExpandAndGetRef();

    defaultSubPassDependency.m_uiSourceSubPass        = XII_GAL_SUBPASS_EXTERNAL;
    defaultSubPassDependency.m_uiDestinationSubPass   = 0U;
    defaultSubPassDependency.m_SourceStageFlags       = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
    defaultSubPassDependency.m_DestinationStageFlags  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
    defaultSubPassDependency.m_DestinationAccessFlags = xiiGALAccessFlags::None;

    if (!m_RenderPassDescription.m_SubPasses[0].m_RenderTargetAttachments.IsEmpty())
    {
      defaultSubPassDependency.m_DestinationAccessFlags |= xiiGALAccessFlags::RenderTargetRead | xiiGALAccessFlags::RenderTargetWrite;
    }
    if (!m_RenderPassDescription.m_SubPasses[0].m_DepthStencilAttachment.IsEmpty())
    {
      defaultSubPassDependency.m_DestinationAccessFlags |= xiiGALAccessFlags::DepthStencilRead | xiiGALAccessFlags::DepthStencilWrite;
    }
  }

  // The framebuffer descriptor’s render pass pointer is typically set after the render pass is created through the device. For now, leave it null.
  m_FramebufferDescription.m_pRenderPass = nullptr;
}

void xiiRenderingSetup::Reset()
{
  *this = xiiRenderingSetup();
}

void xiiRenderingSetup::AddColorAttachment(const xiiSharedPtr<xiiGALTextureView>& pView, xiiEnum<xiiGALAttachmentLoadOperation> loadOp, xiiEnum<xiiGALAttachmentStoreOperation> storeOp, xiiBitflags<xiiGALResourceStateFlags> subpassState, const xiiColor& clearColor)
{
  Attachment& attachment               = m_Attachments.ExpandAndGetRef();
  attachment.m_pView                   = pView;
  attachment.m_LoadOp                  = loadOp;
  attachment.m_StoreOp                 = storeOp;
  attachment.m_SubpassState            = subpassState;
  attachment.m_ClearValue.m_ClearColor = clearColor;
}

void xiiRenderingSetup::AddDepthStencilAttachment(const xiiSharedPtr<xiiGALTextureView>& pView, xiiEnum<xiiGALAttachmentLoadOperation> loadOp, xiiEnum<xiiGALAttachmentStoreOperation> storeOp, xiiEnum<xiiGALAttachmentLoadOperation> stencilLoadOp, xiiEnum<xiiGALAttachmentStoreOperation> stencilStoreOp, xiiBitflags<xiiGALResourceStateFlags> subpassState, float fDepthClear, xiiUInt8 uiStencilClear)
{
  Attachment& attachment                             = m_Attachments.ExpandAndGetRef();
  attachment.m_pView                                 = pView;
  attachment.m_LoadOp                                = loadOp;
  attachment.m_StoreOp                               = storeOp;
  attachment.m_SubpassState                          = subpassState;
  attachment.m_StencilLoadOp                         = stencilLoadOp;
  attachment.m_StencilStoreOp                        = stencilStoreOp;
  attachment.m_ClearValue.m_DepthStencil.m_fDepth    = fDepthClear;
  attachment.m_ClearValue.m_DepthStencil.m_uiStencil = uiStencilClear;
}

void xiiRenderingSetup::DeduceFramebufferSize(const xiiSharedPtr<xiiGALTextureView>& pView)
{
  const auto& textureDescription = pView->GetTexture()->GetDescription();
  const auto& viewDescription    = pView->GetDescription();

  if (m_FramebufferDescription.m_FramebufferSize == xiiSizeU32(0, 0))
  {
    m_FramebufferDescription.m_FramebufferSize   = textureDescription.m_Size;
    m_FramebufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
  }
  else
  {
    XII_ASSERT_DEBUG(textureDescription.m_Size == m_FramebufferDescription.m_FramebufferSize, "Texture size mismatch detected in xiiRenderingSetup!");
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_RenderTargetSetup);
