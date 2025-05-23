#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>

#include <GraphicsCore/Pipeline/Passes/AntialiasingPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAntialiasingPass, 1, xiiRTTIDefaultAllocator<xiiAntialiasingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Post Processing")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAntialiasingPass::xiiAntialiasingPass() :
  xiiRenderPipelinePass("AntialiasingPass", true)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Antialiasing.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load antialiasing shader!");
  }
}

xiiAntialiasingPass::~xiiAntialiasingPass() = default;

bool xiiAntialiasingPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_uiSampleCount == (xiiUInt32)xiiGALMSAASampleCount::TwoSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
    }
    else if (pInput->m_uiSampleCount == (xiiUInt32)xiiGALMSAASampleCount::FourSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
    }
    else if (pInput->m_uiSampleCount == (xiiUInt32)xiiGALMSAASampleCount::EightSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
    }
    else
    {
      xiiLog::Error("Input is not a valid msaa target");
      return false;
    }

    xiiGALTextureCreationDescription desc = *pInput;
    desc.m_uiSampleCount                  = (xiiUInt32)xiiGALMSAASampleCount::OneSample;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiAntialiasingPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pInput  = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
    return;

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pOutput->m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));

  // Bind render target and viewport
  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(nullptr, nullptr, nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

xiiResult xiiAntialiasingPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiAntialiasingPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_AntialiasingPass);
