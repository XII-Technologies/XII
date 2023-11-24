#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/SourcePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSourcePass, 3, xiiRTTIDefaultAllocator<xiiSourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("MSAA_Mode", xiiGALMSAASampleCount, m_MsaaMode),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("Clear", m_bClear),
  }
  XII_END_PROPERTIES;
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

xiiSourcePass::xiiSourcePass(const char* szName) :
  xiiRenderPipelinePass(szName, true)
{
  m_Format     = xiiSourceFormat::Default;
  m_MsaaMode   = xiiGALMSAASampleCount::None;
  m_bClear     = true;
  m_ClearColor = xiiColor::Black;
}

xiiSourcePass::~xiiSourcePass() = default;

bool xiiSourcePass::GetRenderTargetDescriptions(
  const xiiView&                                             view,
  const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
  xiiArrayPtr<xiiGALTextureCreationDescription>              outputs)
{
  xiiUInt32 uiWidth  = static_cast<xiiUInt32>(view.GetViewport().width);
  xiiUInt32 uiHeight = static_cast<xiiUInt32>(view.GetViewport().height);

  xiiGALDevice*              pDevice       = xiiGALDevice::GetDefaultDevice();
  const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  xiiGALTextureCreationDescription desc;

  // Color
  if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized || m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
  {
    xiiEnum<xiiGALTextureFormat> preferredFormat = xiiGALTextureFormat::Unknown;
    if (const xiiGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      auto rendertargetDesc = pTexture->GetDescription();

      preferredFormat = rendertargetDesc.m_Format;
    }

    switch (preferredFormat)
    {
      case xiiGALResourceFormat::RGBAUByteNormalized:
      case xiiGALResourceFormat::RGBAUByteNormalizedsRGB:
      default:
        if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = xiiGALResourceFormat::RGBAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = xiiGALResourceFormat::RGBAUByteNormalized;
        }
        break;
      case xiiGALResourceFormat::BGRAUByteNormalized:
      case xiiGALResourceFormat::BGRAUByteNormalizedsRGB:
        if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = xiiGALResourceFormat::BGRAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = xiiGALResourceFormat::BGRAUByteNormalized;
        }
        break;
    }
  }
  else
  {
    switch (m_Format)
    {
      case xiiSourceFormat::Color4Channel16BitFloat:
        desc.m_Format = xiiGALResourceFormat::RGBAHalf;
        break;
      case xiiSourceFormat::Color4Channel32BitFloat:
        desc.m_Format = xiiGALResourceFormat::RGBAFloat;
        break;
      case xiiSourceFormat::Color3Channel11_11_10BitFloat:
        desc.m_Format = xiiGALResourceFormat::RG11B10Float;
        break;
      case xiiSourceFormat::Depth16Bit:
        desc.m_Format = xiiGALResourceFormat::D16;
        break;
      case xiiSourceFormat::Depth24BitStencil8Bit:
        desc.m_Format = xiiGALResourceFormat::D24S8;
        break;
      case xiiSourceFormat::Depth32BitFloat:
        desc.m_Format = xiiGALResourceFormat::DFloat;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED
    }
  }

  desc.m_uiWidth             = uiWidth;
  desc.m_uiHeight            = uiHeight;
  desc.m_SampleCount         = m_MsaaMode;
  desc.m_bCreateRenderTarget = true;
  desc.m_uiArraySize         = view.GetCamera()->IsStereoscopic() ? 2 : 1;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void xiiSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor              = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth             = true;
  renderingSetup.m_bClearStencil           = true;

  if (xiiGALResourceFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

xiiResult xiiSourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_bClear;
  return XII_SUCCESS;
}

xiiResult xiiSourcePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;
  inout_stream >> m_bClear;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Reflection/ReflectionUtils.h>

class xiiSourcePassPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSourcePassPatch_1_2() :
    xiiGraphPatch("xiiSourcePass", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

xiiSourcePassPatch_1_2 g_xiiSourcePassPatch_1_2;

class xiiSourcePassPatch_2_3 : public xiiGraphPatch
{
public:
  xiiSourcePassPatch_2_3() :
    xiiGraphPatch("xiiSourcePass", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    xiiAbstractObjectNode::Property* formatProperty = pNode->FindProperty("Format");
    if (formatProperty == nullptr)
      return;

    auto                          formatName = formatProperty->m_Value.Get<xiiString>();
    xiiEnum<xiiGALResourceFormat> oldFormat;
    xiiReflectionUtils::StringToEnumeration<xiiGALResourceFormat>(formatName.GetData(), oldFormat);

    xiiEnum<xiiSourceFormat> newFormat;

    switch (oldFormat)
    {
      case xiiGALResourceFormat::RGBAHalf:
        newFormat = xiiSourceFormat::Color4Channel16BitFloat;
        break;
      case xiiGALResourceFormat::RGBAFloat:
        newFormat = xiiSourceFormat::Color4Channel32BitFloat;
        break;
      case xiiGALResourceFormat::RG11B10Float:
        newFormat = xiiSourceFormat::Color3Channel11_11_10BitFloat;
        break;
      case xiiGALResourceFormat::D16:
        newFormat = xiiSourceFormat::Depth16Bit;
        break;
      case xiiGALResourceFormat::D24S8:
        newFormat = xiiSourceFormat::Depth24BitStencil8Bit;
        break;
      case xiiGALResourceFormat::DFloat:
        newFormat = xiiSourceFormat::Depth32BitFloat;
        break;
      case xiiGALResourceFormat::RGBAUByteNormalized:
      case xiiGALResourceFormat::BGRAUByteNormalized:
        newFormat = xiiSourceFormat::Color4Channel8BitNormalized;
        break;
      case xiiGALResourceFormat::RGBAUByteNormalizedsRGB:
      case xiiGALResourceFormat::BGRAUByteNormalizedsRGB:
        newFormat = xiiSourceFormat::Color4Channel8BitNormalized_sRGB;
        break;
      default:
        newFormat = xiiSourceFormat::Default;
        break;
    }

    xiiStringBuilder newFormatName;
    xiiReflectionUtils::EnumerationToString(newFormat, newFormatName);
    formatProperty->m_Value = newFormatName.GetView();
  }
};

xiiSourcePassPatch_2_3 g_xiiSourcePassPatch_2_3;


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SourcePass);
