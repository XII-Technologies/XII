#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/SourcePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSourcePass, 1, xiiRTTIDefaultAllocator<xiiSourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount),
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

xiiSourcePass::xiiSourcePass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  m_Format      = xiiSourceFormat::Default;
  m_SampleCount = xiiGALMSAASampleCount::OneSample;
  m_bClear      = true;
  m_ClearColor  = xiiColor::Black;
}

xiiSourcePass::~xiiSourcePass() = default;

bool xiiSourcePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  xiiUInt32 uiWidth  = static_cast<xiiUInt32>(view.GetViewport().width);
  xiiUInt32 uiHeight = static_cast<xiiUInt32>(view.GetViewport().height);

  xiiGALDevice*              pDevice       = xiiGALDevice::GetDefaultDevice();
  const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  xiiGALTextureCreationDescription desc;
  desc.m_Type = xiiGALResourceDimension::Texture2D;

  // Color
  if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized || m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
  {
    xiiGALTextureFormat::Enum preferredFormat = xiiGALTextureFormat::Unknown;
    if (const xiiGALTextureView* pTextureView = pDevice->GetTextureView(renderTargets.m_hRTs[0]))
    {
      auto rendertargetDesc = pTextureView->GetTexture()->GetDescription();

      preferredFormat = rendertargetDesc.m_Format;
    }

    switch (preferredFormat)
    {
      case xiiGALTextureFormat::RGBA8UNormalized:
      case xiiGALTextureFormat::RGBA8UNormalizedSRGB:
      default:
        if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = xiiGALTextureFormat::RGBA8UNormalizedSRGB;
        }
        else
        {
          desc.m_Format = xiiGALTextureFormat::RGBA8UNormalized;
        }
        break;
      case xiiGALTextureFormat::BGRA8UNormalized:
      case xiiGALTextureFormat::BGRA8UNormalizedSRGB:
        if (m_Format == xiiSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = xiiGALTextureFormat::BGRA8UNormalizedSRGB;
        }
        else
        {
          desc.m_Format = xiiGALTextureFormat::BGRA8UNormalized;
        }
        break;
    }
  }
  else
  {
    switch (m_Format)
    {
      case xiiSourceFormat::Color4Channel16BitFloat:
        desc.m_Format = xiiGALTextureFormat::RGBA16Float;
        break;
      case xiiSourceFormat::Color4Channel32BitFloat:
        desc.m_Format = xiiGALTextureFormat::RGBA32Float;
        break;
      case xiiSourceFormat::Color3Channel11_11_10BitFloat:
        desc.m_Format = xiiGALTextureFormat::RG11B10Float;
        break;
      case xiiSourceFormat::Depth16Bit:
        desc.m_Format = xiiGALTextureFormat::D16UNormalized;
        break;
      case xiiSourceFormat::Depth24BitStencil8Bit:
        desc.m_Format = xiiGALTextureFormat::D24UNormalizedS8UInt;
        break;
      case xiiSourceFormat::Depth32BitFloat:
        desc.m_Format = xiiGALTextureFormat::D32Float;
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  desc.m_Size.width         = uiWidth;
  desc.m_Size.height        = uiHeight;
  desc.m_uiSampleCount      = m_SampleCount;
  desc.m_uiArraySizeOrDepth = view.GetCamera()->IsStereoscopic() ? 2 : 1;
  desc.m_BindFlags          = ((!xiiGALTextureFormat::IsDepthFormat(desc.m_Format) ? xiiGALBindFlags::RenderTarget : xiiGALBindFlags::DepthStencil) | xiiGALBindFlags::ShaderResource);

  if (desc.m_uiArraySizeOrDepth > 1 || desc.m_uiSampleCount > xiiGALMSAASampleCount::OneSample)
    desc.m_Type = xiiGALResourceDimension::Texture2DArray;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void xiiSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor              = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFFU;
  renderingSetup.m_bClearDepth             = true;
  renderingSetup.m_bClearStencil           = true;

  if (xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }

  auto pCommandList = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

xiiResult xiiSourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_SampleCount;
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
  inout_stream >> m_SampleCount;
  inout_stream >> m_ClearColor;
  inout_stream >> m_bClear;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SourcePass);
