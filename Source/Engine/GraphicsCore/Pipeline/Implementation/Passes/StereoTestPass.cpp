#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/StereoTestPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>


#include <GraphicsFoundation/Resources/Texture.h>

#include <Core/Graphics/Camera.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStereoTestPass, 1, xiiRTTIDefaultAllocator<xiiStereoTestPass>)
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

xiiStereoTestPass::xiiStereoTestPass() :
  xiiRenderPipelinePass("StereoTestPass", true)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/StereoTest.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load stereo test shader!");
  }
}

xiiStereoTestPass::~xiiStereoTestPass() = default;

bool xiiStereoTestPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    xiiGALTextureCreationDescription desc = *pInput;
    desc.m_uiSampleCount                  = xiiGALMSAASampleCount::OneSample;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiStereoTestPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
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
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));

  // Bind render target and viewport
  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetTexture(pInput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::ShaderResource));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_StereoTestPass);
