#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/BlurPass.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/Effects/Blur/BlurConstants.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiBlurType, 1)
  XII_ENUM_CONSTANT(xiiBlurType::Box),
  XII_ENUM_CONSTANT(xiiBlurType::Gaussian),
  XII_ENUM_CONSTANT(xiiBlurType::Kawase),
  XII_ENUM_CONSTANT(xiiBlurType::DualKawase),
  XII_ENUM_CONSTANT(xiiBlurType::Bilateral),
  XII_ENUM_CONSTANT(xiiBlurType::Bokeh),
  XII_ENUM_CONSTANT(xiiBlurType::Directional),
  XII_ENUM_CONSTANT(xiiBlurType::Radial),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlurSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBlurSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Type", xiiBlurType, m_Type)->AddAttributes(new xiiDefaultValueAttribute(xiiBlurType::Gaussian)),
    XII_MEMBER_PROPERTY("RadiusPixels", m_fRadiusPixels)->AddAttributes(new xiiDefaultValueAttribute(6.0f)),
    XII_MEMBER_PROPERTY("Sigma", m_fSigma)->AddAttributes(new xiiDefaultValueAttribute(3.0f)),
    XII_MEMBER_PROPERTY("IterationCount", m_uiIterationCount)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("Direction", m_vDirection)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2::MakeZero())),
    XII_MEMBER_PROPERTY("RadialCenter", m_vRadialCenter)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(0.5f, 0.5f))),
    XII_MEMBER_PROPERTY("RadialStrength", m_fRadialStrength)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("DepthSigma", m_fDepthSigma)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("NormalSigma", m_fNormalSigma)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("UseDownsample", m_bUseDownsample)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("MaxMip", m_uiMaxMip)->AddAttributes(new xiiDefaultValueAttribute(2U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlurPass, 1, xiiRTTIDefaultAllocator<xiiBlurPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("DepthTexture", m_PinDepthTexture),
    XII_MEMBER_PROPERTY("NormalTexture", m_PinNormalTexture),
    XII_MEMBER_PROPERTY("MotionVectors", m_PinMotionVectors),
    XII_MEMBER_PROPERTY("CocBuffer", m_PinCoCBuffer),
    XII_MEMBER_PROPERTY("MaskTexture", m_PinMaskTexture),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBlurPass::xiiBlurPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Effects/Blur/Blur.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Failed to load blur shader!");
  }

  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    m_pBlurConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiBlurConstants), "xiiBlurConstants");
  }
}

xiiBlurPass::~xiiBlurPass() = default;

xiiResult xiiBlurPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  return XII_SUCCESS;
}

xiiResult xiiBlurPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  return XII_SUCCESS;
}

xiiResult xiiBlurPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinInput.m_uiInputIndex])
  {
    if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount != static_cast<xiiUInt32>(xiiGALMSAASampleCount::OneSample))
    {
      xiiLog::Error("Input texture must be a non-MSAA source in pass '{0}'!", GetName());
      return XII_FAILURE;
    }
    pOutputs[m_PinOutput.m_uiOutputIndex] = *pInputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No input colour attachment connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiBlurPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pInputColourAttachment = pInputs[m_PinInput.m_uiInputIndex];
  if (pInputColourAttachment == nullptr)
    return;

  auto pOutputColourAttachment = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)}).Build();

  auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  {
    const xiiGALTextureCreationDescription& sourceTextureDescription = pInputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDescription();

    xiiGALMapHelper<xiiBlurConstants> pBlurConstants(pRenderContext->GetCommandList(), m_pBlurConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
    pBlurConstants->SourceTexelSize = {1.0f / sourceTextureDescription.GetWidth(), 1.0f / sourceTextureDescription.GetHeight()};
    pBlurConstants->Direction       = m_BlurSettings.m_vDirection;
    pBlurConstants->RadialCenter    = m_BlurSettings.m_vRadialCenter;
    pBlurConstants->RadiusPixels    = m_BlurSettings.m_fRadiusPixels;
    pBlurConstants->Sigma           = m_BlurSettings.m_fSigma;
    pBlurConstants->RadialStrength  = m_BlurSettings.m_fRadialStrength;
    pBlurConstants->Iterations      = m_BlurSettings.m_uiIterationCount;
    pBlurConstants->DepthSigma      = m_BlurSettings.m_fDepthSigma;
    pBlurConstants->NormalSigma     = m_BlurSettings.m_fNormalSigma;

    xiiStaticArray<float, 32U> weights, offsets;
    PrepareKernel(weights, offsets);

    xiiMemoryUtils::Copy(pBlurConstants->KernelWeights, weights.GetData(), weights.GetCount() * sizeof(float));
    xiiMemoryUtils::Copy(pBlurConstants->KernelOffsets, offsets.GetData(), offsets.GetCount() * sizeof(float));
  }

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer(XII_PP_STRINGIFY(xiiBlurConstants), m_pBlurConstantBuffer);
  pRenderContext->BindTexture("colorTexture", pInputColourAttachment->m_Resource.m_Texture.m_pTexture);
  pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1U);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}

void xiiBlurPass::PrepareKernel(xiiStaticArray<float, 32U>& out_weights, xiiStaticArray<float, 32U>& out_offsets)
{
  out_weights.Clear();
  out_offsets.Clear();

  switch (m_BlurSettings.m_Type)
  {
    case xiiBlurType::Gaussian:
    {
      const xiiUInt32 uiRadius = static_cast<xiiUInt32>(xiiMath::Clamp(xiiMath::Ceil(3.0f * m_BlurSettings.m_fSigma), 1.0f, 15.0f));

      // Raw weights
      xiiDynamicArray<float> raw;
      raw.Reserve(uiRadius + 1);
      for (xiiUInt32 i = 0; i <= uiRadius; ++i)
      {
        const float fSampleOffset = static_cast<float>(i);

        raw.PushBack(xiiMath::Exp(-(fSampleOffset * fSampleOffset) / (2.0f * m_BlurSettings.m_fSigma * m_BlurSettings.m_fSigma)));
      }

      // Normalize
      float fWeight = raw[0];
      for (xiiUInt32 i = 1; i <= uiRadius; ++i)
      {
        fWeight += 2.0f * raw[i];
      }
      for (xiiUInt32 i = 0; i <= uiRadius; ++i)
      {
        raw[i] /= fWeight;
      }

      // Paired taps
      out_weights.PushBack(raw[0]);
      out_offsets.PushBack(0.0f);
      for (xiiUInt32 i = 1; i <= uiRadius; ++i)
      {
        out_weights.PushBack(2.0f * raw[i]);
        out_offsets.PushBack(static_cast<float>(i));
      }
    }
    break;
    case xiiBlurType::Box:
    {
      const xiiUInt32 uiRadius = static_cast<xiiUInt32>(m_BlurSettings.m_fRadiusPixels);
      const float     fWeight  = 1.0f / (2.0f * uiRadius + 1.0f);

      out_weights.PushBack(fWeight);
      out_offsets.PushBack(0.0f);

      for (xiiUInt32 i = 1U; i <= uiRadius; ++i)
      {
        out_weights.PushBack(2.0f * fWeight);
        out_offsets.PushBack(static_cast<float>(i));
      }
    }
    break;
    case xiiBlurType::Kawase:
    {
      // Kawase uses fixed offset pattern per iteration.
      const float r = m_BlurSettings.m_fRadiusPixels + static_cast<float>(m_BlurSettings.m_uiIterationCount);
      const float k = 0.25f; // 4 taps.

      out_weights.PushBack(k);
      out_offsets.PushBack(r);
      out_weights.PushBack(k);
      out_offsets.PushBack(-r);
      out_weights.PushBack(k);
      out_offsets.PushBack(r); // Axis select in shader.
      out_weights.PushBack(k);
      out_offsets.PushBack(-r);
    }
    break;
    case xiiBlurType::Directional:
    {
      // Sample along a vector direction.
      const xiiUInt32 uiTapCount = 8U;
      const float     fStep      = m_BlurSettings.m_fRadiusPixels / float(uiTapCount);
      const float     fWeight    = 1.0f / uiTapCount;

      for (xiiUInt32 i = 0U; i < uiTapCount; ++i)
      {
        out_weights.PushBack(fWeight);
        out_offsets.PushBack(fStep * (i + 1));
      }
      break;
    }
    break;
    default:
    {
      // Other blur types (bilateral, radial, dual Kawase, bokeh) often compute weights in shader per pixel,
      // so kernel prep may be trivial or skipped.
      out_weights.PushBack(1.0f);
      out_offsets.PushBack(0.0f);
    }
    break;
  }
}
