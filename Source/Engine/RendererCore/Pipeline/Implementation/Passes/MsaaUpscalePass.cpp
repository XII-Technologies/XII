#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsaaUpscalePass, 2, xiiRTTIDefaultAllocator<xiiMsaaUpscalePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("MSAA_Mode", xiiGALMSAASampleCount, m_MsaaMode)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMsaaUpscalePass::xiiMsaaUpscalePass() :
  xiiRenderPipelinePass("MsaaUpscalePass"), m_MsaaMode(xiiGALMSAASampleCount::None)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaUpscale.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

xiiMsaaUpscalePass::~xiiMsaaUpscalePass() {}

bool xiiMsaaUpscalePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount != xiiGALMSAASampleCount::None)
    {
      xiiLog::Error("Input must not be a msaa target");
      return false;
    }

    xiiGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount                    = m_MsaaMode;
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

void xiiMsaaUpscalePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
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

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiMsaaUpscalePassPatch_1_2 : public xiiGraphPatch
{
public:
  xiiMsaaUpscalePassPatch_1_2() :
    xiiGraphPatch("xiiMsaaUpscalePass", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override { pNode->RenameProperty("MSAA Mode", "MSAA_Mode"); }
};

xiiMsaaUpscalePassPatch_1_2 g_xiiMsaaUpscalePassPatch_1_2;



XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
