#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/SourcePass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSourcePass, 1, xiiRTTIDefaultAllocator<xiiSourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount),
    XII_ENUM_MEMBER_PROPERTY("LoadOperation", xiiGALAttachmentLoadOperation, m_AttachmentLoadOperation),
    XII_ENUM_MEMBER_PROPERTY("StoreOperation", xiiGALAttachmentStoreOperation, m_AttachmentStoreOperation),
    XII_ENUM_MEMBER_PROPERTY("StencilLoadOperation", xiiGALAttachmentLoadOperation, m_AttachmentStencilLoadOperation),
    XII_ENUM_MEMBER_PROPERTY("StencilStoreOperation", xiiGALAttachmentStoreOperation, m_AttachmentStencilStoreOperation),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("DepthClearValue", m_fDepthClearValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("StencilClearValue", m_uiStencilClearValue)->AddAttributes(new xiiDefaultValueAttribute(0U)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSourceFormat, 1)
  XII_ENUM_CONSTANTS(
    xiiSourceFormat::Color4Channel8BitNormalized_sRGB,
    xiiSourceFormat::Color4Channel8BitNormalized,
    xiiSourceFormat::Color4Channel16BitFloat,
    xiiSourceFormat::Color4Channel32BitFloat,
    xiiSourceFormat::Color3Channel11_11_10BitFloat,
    xiiSourceFormat::Depth16Bit,
    xiiSourceFormat::Depth24BitStencil8Bit,
    xiiSourceFormat::Depth32BitFloat
  )
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiSourcePass::xiiSourcePass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
}

xiiSourcePass::~xiiSourcePass()
{
  m_pFramebuffer.Clear();
  m_pRenderPass.Clear();
}

xiiGALTextureCreationDescription xiiSourcePass::GetOutputDescription(const xiiView& view, xiiEnum<xiiSourceFormat> format, xiiEnum<xiiGALMSAASampleCount> msaaSampleCount)
{
  xiiUInt32 uiWidth  = static_cast<xiiUInt32>(view.GetViewport().width);
  xiiUInt32 uiHeight = static_cast<xiiUInt32>(view.GetViewport().height);

  xiiSharedPtr<xiiGALDevice> pDevice       = xiiGALDevice::GetDefaultDevice();
  const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type = xiiGALResourceDimension::Texture2DArray;

  // Color
  if (format == xiiSourceFormat::Color4Channel8BitNormalized || format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
  {
    xiiGALResourceFormat::Enum preferredFormat = xiiGALResourceFormat::Unknown;
    if (renderTargets.m_pRTs[0])
    {
      auto rendertargetDesc = renderTargets.m_pRTs[0]->GetTexture()->GetDescription();

      preferredFormat = rendertargetDesc.m_Format;
    }

    switch (preferredFormat)
    {
      case xiiGALResourceFormat::RGBA8UNormalized:
      case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      default:
        textureDescription.m_Format = (format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB) ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : xiiGALResourceFormat::RGBA8UNormalized;
        break;
      case xiiGALResourceFormat::BGRA8UNormalized:
      case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
        textureDescription.m_Format = (format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB) ? xiiGALResourceFormat::BGRA8UNormalizedSRGB : xiiGALResourceFormat::BGRA8UNormalized;
        break;
    }
  }
  else
  {
    switch (format)
    {
      case xiiSourceFormat::Color4Channel16BitFloat:
        textureDescription.m_Format = xiiGALResourceFormat::RGBA16Float;
        break;
      case xiiSourceFormat::Color4Channel32BitFloat:
        textureDescription.m_Format = xiiGALResourceFormat::RGBA32Float;
        break;
      case xiiSourceFormat::Color3Channel11_11_10BitFloat:
        textureDescription.m_Format = xiiGALResourceFormat::RG11B10Float;
        break;
      case xiiSourceFormat::Depth16Bit:
        textureDescription.m_Format = xiiGALResourceFormat::D16UNormalized;
        break;
      case xiiSourceFormat::Depth24BitStencil8Bit:
        textureDescription.m_Format = xiiGALResourceFormat::D24UNormalizedS8UInt;
        break;
      case xiiSourceFormat::Depth32BitFloat:
        textureDescription.m_Format = xiiGALResourceFormat::D32Float;
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  textureDescription.m_Size.width         = uiWidth;
  textureDescription.m_Size.height        = uiHeight;
  textureDescription.m_uiSampleCount      = msaaSampleCount.GetValue();
  textureDescription.m_uiArraySizeOrDepth = view.GetCamera()->IsStereoscopic() ? 2 : 1;
  textureDescription.m_BindFlags          = ((!xiiGALResourceFormat::IsDepthFormat(textureDescription.m_Format) ? xiiGALBindFlags::RenderTarget : xiiGALBindFlags::DepthStencil) | xiiGALBindFlags::ShaderResource);

  return textureDescription;
}

bool xiiSourcePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);
  pOutputs[m_PinOutput.m_uiOutputIndex] = GetOutputDescription(view, m_Format, m_SampleCount);
  return true;
}

void xiiSourcePass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Create render pass.
  if (auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex])
  {
    const auto& textureDescription = pOutput->m_pTexture->GetDescription();
    const bool  bIsDepthAttachment = xiiGALResourceFormat::IsDepthFormat(pOutput->m_TextureDescription.m_Format);

    xiiGALRenderPassCreationDescription renderPassDescription;
    auto&                               subpassDescription    = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    auto&                               dependencyDescription = renderPassDescription.m_Dependencies.ExpandAndGetRef();
    auto&                               attachmentDescription = renderPassDescription.m_Attachments.ExpandAndGetRef();

    dependencyDescription.m_uiSourceSubPass       = XII_GAL_SUBPASS_EXTERNAL;
    dependencyDescription.m_uiDestinationSubPass  = 0U;
    dependencyDescription.m_SourceStageFlags      = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
    dependencyDescription.m_DestinationStageFlags = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;

    attachmentDescription.m_Format                = textureDescription.m_Format;
    attachmentDescription.m_uiSampleCount         = m_SampleCount;
    attachmentDescription.m_LoadOperation         = m_AttachmentLoadOperation;
    attachmentDescription.m_StoreOperation        = m_AttachmentStoreOperation;
    attachmentDescription.m_StencilLoadOperation  = m_AttachmentStencilLoadOperation;
    attachmentDescription.m_StencilStoreOperation = m_AttachmentStencilStoreOperation;
    attachmentDescription.m_InitialStateFlags     = bIsDepthAttachment ? xiiGALResourceStateFlags::DepthWrite : xiiGALResourceStateFlags::RenderTarget;
    attachmentDescription.m_FinalStateFlags       = bIsDepthAttachment ? xiiGALResourceStateFlags::DepthWrite : xiiGALResourceStateFlags::RenderTarget;

    if (bIsDepthAttachment)
    {
      auto& depthAttachmentReference                = subpassDescription.m_DepthStencilAttachment.ExpandAndGetRef();
      depthAttachmentReference.m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentReference.m_uiAttachmentIndex  = 0U;

      dependencyDescription.m_SourceAccessFlags      = xiiGALAccessFlags::DepthStencilWrite;
      dependencyDescription.m_DestinationAccessFlags = xiiGALAccessFlags::DepthStencilWrite;
    }
    else
    {
      auto& colorAttachmentReference                = subpassDescription.m_RenderTargetAttachments.ExpandAndGetRef();
      colorAttachmentReference.m_ResourceStateFlags = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentReference.m_uiAttachmentIndex  = 0U;

      dependencyDescription.m_SourceAccessFlags      = xiiGALAccessFlags::RenderTargetWrite;
      dependencyDescription.m_DestinationAccessFlags = xiiGALAccessFlags::RenderTargetWrite;
    }

    m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);
    XII_ASSERT_DEV(m_pRenderPass != nullptr, "Failed to create render pass.");
  }
}

void xiiSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  if (!m_pRenderPass)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice            = xiiGALDevice::GetDefaultDevice();
  const bool                 bIsDepthAttachment = xiiGALResourceFormat::IsDepthFormat(pOutput->m_TextureDescription.m_Format);

  if (m_pFramebuffer)
  {
    const auto& pAttachmentView = pOutput->m_pTexture->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);

    if (m_pFramebuffer->GetDescription().m_Attachments.PeekBack() != pAttachmentView)
    {
      m_pFramebuffer.Clear();
    }
  }

  if (!m_pFramebuffer)
  {
    const auto& attachmentDescription     = pOutput->m_pTexture->GetDescription();
    const auto& pAttachmentView           = pOutput->m_pTexture->GetDefaultView(bIsDepthAttachment ? xiiGALTextureViewType::DepthStencil : xiiGALTextureViewType::RenderTarget);
    const auto& attachmentViewDescription = pAttachmentView->GetDescription();
    xiiVec3U32  vSize                     = xiiGALTextureUtilities::GetMipLevelSize(attachmentViewDescription.m_uiMostDetailedMip, attachmentDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_pRenderPass       = m_pRenderPass;
    framebufferDescription.m_FramebufferSize   = {vSize.x, vSize.y};
    framebufferDescription.m_uiArraySliceCount = attachmentDescription.GetArraySize();
    framebufferDescription.m_Attachments.PushBack(pAttachmentView);

    m_pFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
    XII_ASSERT_DEV(m_pFramebuffer != nullptr, "Failed to create frame buffer.");
  }

  xiiGALBeginRenderPassDescription renderPassDescription = {.m_pRenderPass = m_pRenderPass, .m_pFramebuffer = m_pFramebuffer};

  if (bIsDepthAttachment)
  {
    auto& clearValue                      = renderPassDescription.m_ClearValues.ExpandAndGetRef();
    clearValue.m_ResourceFormat           = pOutput->m_TextureDescription.m_Format;
    clearValue.m_DepthStencil.m_fDepth    = m_fDepthClearValue;
    clearValue.m_DepthStencil.m_uiStencil = m_uiStencilClearValue;
  }
  else
  {
    auto& clearValue            = renderPassDescription.m_ClearValues.ExpandAndGetRef();
    clearValue.m_ResourceFormat = pOutput->m_TextureDescription.m_Format;
    clearValue.m_ClearColor     = m_ClearColor;
  }

  if (auto pCommandList = pDevice->GetDefaultCommandQueue()->BeginCommandList())
  {
    pCommandList->BeginDebugGroup(GetName());
    {
      pCommandList->BeginRenderPass(renderPassDescription);
      pCommandList->EndRenderPass();
    }
    pCommandList->EndDebugGroup();
  }
}

xiiResult xiiSourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Format;
  inout_stream << m_SampleCount;
  inout_stream << m_AttachmentLoadOperation;
  inout_stream << m_AttachmentStoreOperation;
  inout_stream << m_AttachmentStencilLoadOperation;
  inout_stream << m_AttachmentStencilStoreOperation;
  inout_stream << m_ClearColor;
  inout_stream << m_fDepthClearValue;
  inout_stream << m_uiStencilClearValue;

  return XII_SUCCESS;
}

xiiResult xiiSourcePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Format;
  inout_stream >> m_SampleCount;
  inout_stream >> m_AttachmentLoadOperation;
  inout_stream >> m_AttachmentStoreOperation;
  inout_stream >> m_AttachmentStencilLoadOperation;
  inout_stream >> m_AttachmentStencilStoreOperation;
  inout_stream >> m_ClearColor;
  inout_stream >> m_fDepthClearValue;
  inout_stream >> m_uiStencilClearValue;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SourcePass);
