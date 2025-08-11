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
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAccessFlags::None)),
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
  const xiiRectFloat& viewport = view.GetViewport();

  const xiiRenderTargets&    renderTargets   = view.GetActiveRenderTargets();
  xiiGALResourceFormat::Enum preferredFormat = xiiGALResourceFormat::Unknown;
  if (const xiiGALTextureView* pTextureView = renderTargets.m_pRTs[0].Borrow())
  {
    auto viewDescription = pTextureView->GetDescription();

    preferredFormat = viewDescription.m_Format;
  }

  const bool bIsFlipped = preferredFormat == xiiGALResourceFormat::BGRA8UNormalized || preferredFormat == xiiGALResourceFormat::BGRA8UNormalizedSRGB;

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type               = m_Type;
  textureDescription.m_Format             = xiiSourceFormat::GetGALResourceFormat(m_Format, bIsFlipped);
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

void xiiCreateColourAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  if (!m_bClear)
    return;

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({.m_pRenderTarget = pOutput->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget), .m_ClearColour = m_ClearColour, .m_LoadOp = xiiGALAttachmentLoadOperation::Clear}).Build();

  renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, GetName());
  renderViewContext.m_pRenderContext->EndRendering();
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
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags",xiiGALBindFlags , m_BindFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget)),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags)->AddAttributes(new xiiDefaultValueAttribute(xiiGALAccessFlags::None)),
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
  const xiiRectFloat& viewport = view.GetViewport();

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type               = m_Type;
  textureDescription.m_Format             = xiiSourceFormat::GetGALResourceFormat(m_Format);
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

void xiiCreateDepthAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  if (!m_bClear)
    return;

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.SetDepthStencilAttachment({.m_pDSTarget = pOutput->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil), .m_fDepthClear = m_fDepthClearValue, .m_uiStencilClear = m_uiStencilClearValue, .m_LoadOp = xiiGALAttachmentLoadOperation::Clear, .m_StencilLoadOp = xiiGALAttachmentLoadOperation::Clear}).Build();

  renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, GetName());
  renderViewContext.m_pRenderContext->EndRendering();
}
