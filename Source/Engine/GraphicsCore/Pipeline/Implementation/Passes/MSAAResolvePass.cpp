#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/MSAAResolvePass.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMSAAResolvePass, 1, xiiRTTIDefaultAllocator<xiiMSAAResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMSAAResolvePass::xiiMSAAResolvePass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::None)
{
  {
    // Load shader.
    m_hDepthResolveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaDepthResolve.xiiShader");
    XII_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Failed to load MSAA depth resolve shader!");
  }
}

xiiMSAAResolvePass::~xiiMSAAResolvePass() = default;

xiiResult xiiMSAAResolvePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinInput.m_uiInputIndex])
  {
    if (pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount == static_cast<xiiUInt32>(xiiGALMSAASampleCount::OneSample))
    {
      xiiLog::Error("Input texture must be a MSAA source in pass '{0}'!", GetName());
      return XII_FAILURE;
    }

    m_bIsDepthResolve = xiiGALResourceFormat::IsDepthFormat(pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_Format);
    m_SampleCount     = static_cast<xiiGALMSAASampleCount::Enum>(pInputs[m_PinInput.m_uiInputIndex]->m_Texture.m_Description.m_uiSampleCount);

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

void xiiMSAAResolvePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pInputColourAttachment = pInputs[m_PinInput.m_uiInputIndex];
  if (pInputColourAttachment == nullptr)
    return;

  auto pOutputColourAttachment = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutputColourAttachment == nullptr)
    return;

  if (m_bIsDepthResolve)
  {
    xiiRenderingSetup renderingSetup;
    renderingSetup.SetDepthStencilAttachment({pOutputColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil)}).Build();

    auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    xiiPassConstants* pPassConstants = pRenderContext->GetPassConstants();
    pPassConstants->MSAASampleCount  = m_SampleCount;

    pRenderContext->BindShader(m_hDepthResolveShader);
    pRenderContext->BindTexture("depthTexture", pInputColourAttachment->m_Resource.m_Texture.m_pTexture);
    pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, 1);
    pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    xiiGALResolveTextureSubresourceDescription resolveDescription;
    resolveDescription.m_uiSourceMipLevel                 = 0;
    resolveDescription.m_uiSourceSlice                    = 0;
    resolveDescription.m_SourceTextureTransitionMode      = xiiGALStateTransitionMode::Transition;
    resolveDescription.m_uiDestinationMipLevel            = 0;
    resolveDescription.m_uiDestinationSlice               = 0;
    resolveDescription.m_DestinationTextureTransitionMode = xiiGALStateTransitionMode::Transition;

    auto pCommandList = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Graphics>(GetName());

    pCommandList->ResolveTextureSubResource(pInputColourAttachment->m_Resource.m_Texture.m_pTexture, pOutputColourAttachment->m_Resource.m_Texture.m_pTexture, resolveDescription);

    if (renderViewContext.m_pCamera->IsStereoscopic())
    {
      resolveDescription.m_uiSourceSlice      = 1;
      resolveDescription.m_uiDestinationSlice = 1;

      pCommandList->ResolveTextureSubResource(pInputColourAttachment->m_Resource.m_Texture.m_pTexture, pOutputColourAttachment->m_Resource.m_Texture.m_pTexture, resolveDescription);
    }
  }
}
