#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateColourAttachmentPass, 1, xiiRTTINoAllocator)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiGALResourceDimension, m_Type)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceDimension::Texture2D)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Color4Channel8BitNormalized_sRGB)),
    XII_MEMBER_PROPERTY("ArraySizeOrDepth", m_uiArraySizeOrDepth)->AddAttributes(new xiiDefaultValueAttribute(0U)),
    XII_MEMBER_PROPERTY("MipLevels", m_uiMipLevels)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags",xiiGALBindFlags , m_BindFlags),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags),
    XII_BITFLAGS_MEMBER_PROPERTY("MiscFlags",xiiGALMiscTextureFlags , m_MiscFlags),
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
}

///////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateDepthAttachmentPass, 1, xiiRTTINoAllocator)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiGALResourceDimension, m_Type)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceDimension::Texture2D)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiSourceFormat::Depth24BitStencil8Bit)),
    XII_MEMBER_PROPERTY("ArraySizeOrDepth", m_uiArraySizeOrDepth)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("MipLevels", m_uiMipLevels)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_BITFLAGS_MEMBER_PROPERTY("BindFlags",xiiGALBindFlags , m_BindFlags),
    XII_BITFLAGS_MEMBER_PROPERTY("AccessFlags",xiiGALCPUAccessFlag , m_AccessFlags),
    XII_BITFLAGS_MEMBER_PROPERTY("MiscFlags",xiiGALMiscTextureFlags , m_MiscFlags),
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
}
