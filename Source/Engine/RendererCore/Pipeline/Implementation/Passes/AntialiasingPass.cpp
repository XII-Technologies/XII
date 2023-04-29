#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/AntialiasingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAntialiasingPass, 1, xiiRTTIDefaultAllocator<xiiAntialiasingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  XII_END_PROPERTIES;
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

xiiAntialiasingPass::~xiiAntialiasingPass() {}

bool xiiAntialiasingPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == xiiGALMSAASampleCount::TwoSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
    }
    else if (pInput->m_SampleCount == xiiGALMSAASampleCount::FourSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
    }
    else if (pInput->m_SampleCount == xiiGALMSAASampleCount::EightSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
    }
    else
    {
      xiiLog::Error("Input is not a valid msaa target");
      return false;
    }

    xiiGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount                    = xiiGALMSAASampleCount::None;
    desc.m_Type                           = xiiGALTextureType::Texture2DArray;

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
  {
    return;
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
