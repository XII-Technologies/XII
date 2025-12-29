#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateColourAttachmentPass, 1, xiiRTTIDefaultAllocator<xiiCreateColourAttachmentPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiGALResourceDimension, m_Type)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceDimension::Texture2D)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Color4Channel8BitNormalized_sRGB)),
    XII_MEMBER_PROPERTY("ArraySizeOrDepth", m_uiArraySizeOrDepth)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("MipLevels", m_uiMipLevels)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags",xiiGALBindFlags , m_BindFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget)),
    XII_ENUM_MEMBER_PROPERTY("Usage", xiiGALResourceUsage, m_Usage)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceUsage::Default)),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALCPUAccessFlag::None)),
    XII_BITFLAGS_MEMBER_PROPERTY("MiscFlags",xiiGALMiscTextureFlags , m_MiscFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALMiscTextureFlags::None)),

    XII_MEMBER_PROPERTY("Clear", m_bClear)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("ClearColour", m_ClearColour)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::Black), new xiiExposeColorAlphaAttribute()),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCreateColourAttachmentPass::xiiCreateColourAttachmentPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCreateColourAttachmentPass::~xiiCreateColourAttachmentPass() = default;

xiiResult xiiCreateColourAttachmentPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Type;
  inout_stream << m_Format;
  inout_stream << m_uiArraySizeOrDepth;
  inout_stream << m_uiMipLevels;
  inout_stream << m_uiSampleCount;
  inout_stream << m_BindFlags;
  inout_stream << m_Usage;
  inout_stream << m_AccessFlags;
  inout_stream << m_MiscFlags;
  inout_stream << m_bClear;
  inout_stream << m_ClearColour;

  return XII_SUCCESS;
}

xiiResult xiiCreateColourAttachmentPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Type;
  inout_stream >> m_Format;
  inout_stream >> m_uiArraySizeOrDepth;
  inout_stream >> m_uiMipLevels;
  inout_stream >> m_uiSampleCount;
  inout_stream >> m_BindFlags;
  inout_stream >> m_Usage;
  inout_stream >> m_AccessFlags;
  inout_stream >> m_MiscFlags;
  inout_stream >> m_bClear;
  inout_stream >> m_ClearColour;

  return XII_SUCCESS;
}

xiiResult xiiCreateColourAttachmentPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  const xiiRectFloat&        viewport        = view.GetViewport();
  const xiiRenderTargets&    renderTargets   = view.GetActiveRenderTargets();
  xiiGALResourceFormat::Enum preferredFormat = xiiGALResourceFormat::Unknown;

  if (const xiiGALTextureView* pTextureView = renderTargets.m_pRTs[0].Borrow())
  {
    const xiiGALTextureViewCreationDescription& viewDescription = pTextureView->GetDescription();
    preferredFormat                                             = viewDescription.m_Format;
  }

  const bool                    bIsFlipped = preferredFormat == xiiGALResourceFormat::BGRA8UNormalized || preferredFormat == xiiGALResourceFormat::BGRA8UNormalizedSRGB;
  xiiEnum<xiiGALResourceFormat> format     = xiiSourceFormat::GetGALResourceFormat(m_Format, bIsFlipped);

  if (xiiGALResourceFormat::IsDepthFormat(format))
  {
    xiiLog::Error("The resource format is not a colour texture format in pass '{}'.", GetName());
    return XII_FAILURE;
  }

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type               = m_Type;
  textureDescription.m_Format             = format;
  textureDescription.m_Size.width         = static_cast<xiiUInt32>(viewport.width);
  textureDescription.m_Size.height        = static_cast<xiiUInt32>(viewport.height);
  textureDescription.m_uiArraySizeOrDepth = m_uiArraySizeOrDepth;
  textureDescription.m_uiMipLevels        = m_uiMipLevels;
  textureDescription.m_uiSampleCount      = m_uiSampleCount;
  textureDescription.m_BindFlags          = m_BindFlags;
  textureDescription.m_Usage              = m_Usage;
  textureDescription.m_CPUAccessFlags     = m_AccessFlags;
  textureDescription.m_MiscFlags          = m_MiscFlags;

  if (textureDescription.m_uiArraySizeOrDepth == 0U)
  {
    textureDescription.m_uiArraySizeOrDepth = 1U;
  }
  if (view.GetCamera()->IsStereoscopic())
  {
    textureDescription.m_uiArraySizeOrDepth = textureDescription.m_uiArraySizeOrDepth * 2U;
  }

  pOutputs[m_PinOutput.m_uiOutputIndex] = xiiRenderPipelinePassResource(m_PinOutput.m_ResourceType, textureDescription);

  return XII_SUCCESS;
}

xiiResult xiiCreateColourAttachmentPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  m_pRenderPass.Clear();
  m_FramebufferCache.Clear();

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return XII_FAILURE;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderPassCreationDescription renderPassDescription;

  xiiGALRenderPassAttachmentDescription& attachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
  attachment.m_Format                               = pOutput->m_Resource.m_Texture.m_Description.m_Format;
  attachment.m_uiSampleCount                        = pOutput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
  attachment.m_LoadOperation                        = m_bClear ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
  attachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
  attachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
  attachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;

  xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses.ExpandAndGetRef();
  subpass.m_RenderTargetAttachments.PushBack(xiiGALAttachmentReferenceDescription{.m_uiAttachmentIndex = 0U, .m_ResourceStateFlags = xiiGALResourceStateFlags::RenderTarget});

  xiiGALSubPassDependencyDescription& dependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
  dependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
  dependency.m_uiDestinationSubPass              = 0U;
  dependency.m_SourceStageFlags                  = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_DestinationStageFlags             = xiiGALPipelineStageFlags::EarlyFragmentTests | xiiGALPipelineStageFlags::RenderTarget;
  dependency.m_SourceAccessFlags                 = xiiGALAccessFlags::RenderTargetRead | xiiGALAccessFlags::RenderTargetWrite;
  dependency.m_DestinationAccessFlags            = xiiGALAccessFlags::RenderTargetRead | xiiGALAccessFlags::RenderTargetWrite;

  m_pRenderPass = pDevice->CreateRenderPass(renderPassDescription);

  return XII_SUCCESS;
}

void xiiCreateColourAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  if (!m_bClear)
    return;

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiSharedPtr<xiiGALTextureView>             pRenderTarget      = pOutput->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  const xiiGALTextureCreationDescription&     textureDescription = pOutput->m_Resource.m_Texture.m_pTexture->GetDescription();
  const xiiGALTextureViewCreationDescription& viewDescription    = pRenderTarget->GetDescription();

  // Create or retrieve a corresponding framebuffer from the cache if present.
  xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
  {
    xiiGALFramebufferCreationDescription frameBufferDescription;
    frameBufferDescription.m_pRenderPass       = m_pRenderPass;
    frameBufferDescription.m_FramebufferSize   = textureDescription.m_Size;
    frameBufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
    frameBufferDescription.m_Attachments.PushBack(pRenderTarget);

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

  auto pCommandList = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Graphics>(GetName());

  xiiGALOptimizedClearValue clearValue;
  clearValue.m_ClearColour = m_ClearColour;

  pCommandList->BeginRenderPass({m_pRenderPass, pFramebuffer, xiiMakeArrayPtr(&clearValue, 1U)});
  pCommandList->EndRenderPass();
}

///////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateDepthAttachmentPass, 1, xiiRTTIDefaultAllocator<xiiCreateDepthAttachmentPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiGALResourceDimension, m_Type)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceDimension::Texture2D)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Depth24BitStencil8Bit)),
    XII_MEMBER_PROPERTY("ArraySizeOrDepth", m_uiArraySizeOrDepth)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("MipLevels", m_uiMipLevels)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags",xiiGALBindFlags , m_BindFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::DepthStencil)),
    XII_ENUM_MEMBER_PROPERTY("Usage", xiiGALResourceUsage, m_Usage)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceUsage::Default)),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALCPUAccessFlag::None)),
    XII_BITFLAGS_MEMBER_PROPERTY("MiscFlags",xiiGALMiscTextureFlags , m_MiscFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALMiscTextureFlags::None)),

    XII_MEMBER_PROPERTY("Clear", m_bClear)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("DepthClearValue", m_fDepthClearValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("StencilClearValue", m_uiStencilClearValue)->AddAttributes(new xiiDefaultValueAttribute(0U)),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCreateDepthAttachmentPass::xiiCreateDepthAttachmentPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCreateDepthAttachmentPass::~xiiCreateDepthAttachmentPass() = default;

xiiResult xiiCreateDepthAttachmentPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Type;
  inout_stream << m_Format;
  inout_stream << m_uiArraySizeOrDepth;
  inout_stream << m_uiMipLevels;
  inout_stream << m_uiSampleCount;
  inout_stream << m_BindFlags;
  inout_stream << m_Usage;
  inout_stream << m_AccessFlags;
  inout_stream << m_MiscFlags;
  inout_stream << m_bClear;
  inout_stream << m_fDepthClearValue;
  inout_stream << m_uiStencilClearValue;

  return XII_SUCCESS;
}

xiiResult xiiCreateDepthAttachmentPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Type;
  inout_stream >> m_Format;
  inout_stream >> m_uiArraySizeOrDepth;
  inout_stream >> m_uiMipLevels;
  inout_stream >> m_uiSampleCount;
  inout_stream >> m_BindFlags;
  inout_stream >> m_Usage;
  inout_stream >> m_AccessFlags;
  inout_stream >> m_MiscFlags;
  inout_stream >> m_bClear;
  inout_stream >> m_fDepthClearValue;
  inout_stream >> m_uiStencilClearValue;

  return XII_SUCCESS;
}

xiiResult xiiCreateDepthAttachmentPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  const xiiRectFloat&           viewport = view.GetViewport();
  xiiEnum<xiiGALResourceFormat> format   = xiiSourceFormat::GetGALResourceFormat(m_Format);

  if (!xiiGALResourceFormat::IsDepthFormat(format))
  {
    xiiLog::Error("The resource format is not a depth texture format in pass '{}'.", GetName());
    return XII_FAILURE;
  }

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type               = m_Type;
  textureDescription.m_Format             = format;
  textureDescription.m_Size.width         = static_cast<xiiUInt32>(viewport.width);
  textureDescription.m_Size.height        = static_cast<xiiUInt32>(viewport.height);
  textureDescription.m_uiArraySizeOrDepth = m_uiArraySizeOrDepth;
  textureDescription.m_uiMipLevels        = m_uiMipLevels;
  textureDescription.m_uiSampleCount      = m_uiSampleCount;
  textureDescription.m_BindFlags          = m_BindFlags;
  textureDescription.m_Usage              = m_Usage;
  textureDescription.m_CPUAccessFlags     = m_AccessFlags;
  textureDescription.m_MiscFlags          = m_MiscFlags;

  if (textureDescription.m_uiArraySizeOrDepth == 0U)
  {
    textureDescription.m_uiArraySizeOrDepth = view.GetCamera()->IsStereoscopic() ? 2U : 1U;
  }

  pOutputs[m_PinOutput.m_uiOutputIndex] = xiiRenderPipelinePassResource(m_PinOutput.m_ResourceType, textureDescription);

  return XII_SUCCESS;
}

xiiResult xiiCreateDepthAttachmentPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  m_pRenderPass.Clear();
  m_FramebufferCache.Clear();

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return XII_FAILURE;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALRenderPassCreationDescription renderPassDescription;

  xiiGALRenderPassAttachmentDescription& attachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
  attachment.m_Format                               = pOutput->m_Resource.m_Texture.m_Description.m_Format;
  attachment.m_uiSampleCount                        = pOutput->m_Resource.m_Texture.m_Description.m_uiSampleCount;
  attachment.m_LoadOperation                        = m_bClear ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
  attachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
  attachment.m_StencilLoadOperation                 = m_bClear ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
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

void xiiCreateDepthAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  if (!m_bClear)
    return;

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiSharedPtr<xiiGALTextureView>             pDepthStencil      = pOutput->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil);
  const xiiGALTextureCreationDescription&     textureDescription = pOutput->m_Resource.m_Texture.m_pTexture->GetDescription();
  const xiiGALTextureViewCreationDescription& viewDescription    = pDepthStencil->GetDescription();

  // Create or retrieve a corresponding framebuffer from the cache if present.
  xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
  {
    xiiGALFramebufferCreationDescription frameBufferDescription;
    frameBufferDescription.m_pRenderPass       = m_pRenderPass;
    frameBufferDescription.m_FramebufferSize   = textureDescription.m_Size;
    frameBufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
    frameBufferDescription.m_Attachments.PushBack(pDepthStencil);

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

  auto pCommandList = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Graphics>(GetName());

  xiiGALOptimizedClearValue clearValue;
  clearValue.m_DepthStencil.m_fDepth    = m_fDepthClearValue;
  clearValue.m_DepthStencil.m_uiStencil = m_uiStencilClearValue;

  pCommandList->BeginRenderPass({m_pRenderPass, pFramebuffer, xiiMakeArrayPtr(&clearValue, 1U)});
  pCommandList->EndRenderPass();
}
