#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/MSAAUpscalePass.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMSAAUpscalePass, 1, xiiRTTIDefaultAllocator<xiiMSAAUpscalePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("SampleCount", xiiGALMSAASampleCount, m_SampleCount),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMSAAUpscalePass::xiiMSAAUpscalePass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
  {
    // Load shader.
    m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaUpscale.xiiShader");
    XII_ASSERT_DEV(m_hShader.IsValid(), "Failed to load MSAA upscale shader!");
  }
}

xiiMSAAUpscalePass::~xiiMSAAUpscalePass() = default;

xiiResult xiiMSAAUpscalePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_SampleCount;

  return XII_SUCCESS;
}

xiiResult xiiMSAAUpscalePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_SampleCount;

  return XII_SUCCESS;
}

xiiResult xiiMSAAUpscalePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
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

    xiiRenderPipelinePassResource request           = *pInputs[m_PinInput.m_uiInputIndex];
    request.m_Texture.m_Description.m_uiSampleCount = m_SampleCount.GetValue();
    pOutputs[m_PinOutput.m_uiOutputIndex]           = request;
  }
  else
  {
    xiiLog::Error("No input colour attachment connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiMSAAUpscalePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pInputColourAttachment = pInputs[m_PinInput.m_uiInputIndex];
  if (pInputColourAttachment == nullptr)
    return;

  auto pOutputColourAttachment = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)}).Build();

  auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindTexture("colorTexture", pInputColourAttachment->m_Resource.m_Texture.m_pTexture);
  pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}
