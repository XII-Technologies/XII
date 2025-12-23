#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/AntialiasingPass.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAntialiasingPass, 1, xiiRTTIDefaultAllocator<xiiAntialiasingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAntialiasingPass::xiiAntialiasingPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaUpscale.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Failed to load MSAA upscale shader!");
  }
}

xiiAntialiasingPass::~xiiAntialiasingPass() = default;

xiiResult xiiAntialiasingPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinInput.m_uiInputIndex])
  {
    if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount == static_cast<xiiUInt32>(xiiGALMSAASampleCount::TwoSamples))
    {
      m_SampleCount.Assign("MSAA_SAMPLES_TWO");
    }
    else if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount == static_cast<xiiUInt32>(xiiGALMSAASampleCount::FourSamples))
    {
      m_SampleCount.Assign("MSAA_SAMPLES_FOUR");
    }
    else if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount == static_cast<xiiUInt32>(xiiGALMSAASampleCount::EightSamples))
    {
      m_SampleCount.Assign("MSAA_SAMPLES_EIGHT");
    }
    else
    {
      xiiLog::Error("Input to pass '{0}' is not a multi-sampled texture!", GetName());
      return XII_FAILURE;
    }

    xiiRenderPipelinePassResource request           = *pInputs[m_PinInput.m_uiInputIndex];
    request.m_Texture.m_Description.m_uiSampleCount = xiiGALMSAASampleCount::OneSample;
    pOutputs[m_PinOutput.m_uiOutputIndex]           = request;
  }
  else
  {
    xiiLog::Error("No input colour attachment connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiAntialiasingPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pInputColourAttachment = pInputs[m_PinInput.m_uiInputIndex];
  if (pInputColourAttachment == nullptr)
    return;

  auto pOutputColourAttachment = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)});

  auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_SampleCount);
  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindTexture("colorTexture", pInputColourAttachment->m_Resource.m_Texture.m_pTexture);
  pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}
