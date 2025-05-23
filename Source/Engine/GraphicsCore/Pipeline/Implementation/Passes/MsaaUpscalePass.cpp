#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <GraphicsCore/Pipeline/View.h>

#include <GraphicsFoundation/Resources/Texture.h>

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
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMsaaUpscalePass::xiiMsaaUpscalePass() :
  xiiRenderPipelinePass("MsaaUpscalePass")
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaUpscale.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

xiiMsaaUpscalePass::~xiiMsaaUpscalePass() = default;

bool xiiMsaaUpscalePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_uiSampleCount > xiiGALMSAASampleCount::OneSample)
    {
      xiiLog::Error("Input must not be a msaa target");
      return false;
    }

    xiiGALTextureCreationDescription textureDescription = *pInput;
    textureDescription.m_uiSampleCount                  = m_MsaaMode;

    outputs[m_PinOutput.m_uiOutputIndex] = textureDescription;
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
    return;

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pOutput->m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget));

  // Bind render target and viewport
  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(nullptr, nullptr, nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

xiiResult xiiMsaaUpscalePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_MsaaMode;

  return XII_SUCCESS;
}

xiiResult xiiMsaaUpscalePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_MsaaMode;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
