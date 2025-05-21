#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/MsaaResolvePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsaaResolvePass, 1, xiiRTTIDefaultAllocator<xiiMsaaResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput)
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

xiiMsaaResolvePass::xiiMsaaResolvePass() :
  xiiRenderPipelinePass("MsaaResolvePass", true)
{
  {
    // Load shader.
    m_hDepthResolveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/MsaaDepthResolve.xiiShader");
    XII_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

xiiMsaaResolvePass::~xiiMsaaResolvePass() = default;

bool xiiMsaaResolvePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_uiSampleCount == xiiGALMSAASampleCount::OneSample)
    {
      xiiLog::Error("Input is not a valid msaa target");
      return false;
    }

    m_bIsDepth        = xiiGALResourceFormat::IsDepthFormat(pInput->m_Format);
    m_MsaaSampleCount = (xiiGALMSAASampleCount::Enum)pInput->m_uiSampleCount;

    xiiGALTextureCreationDescription textureDescription = *pInput;
    textureDescription.m_uiSampleCount                  = xiiGALMSAASampleCount::OneSample;

    outputs[m_PinOutput.m_uiOutputIndex] = textureDescription;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiMsaaResolvePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pInput  = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
    return;

  if (m_bIsDepth)
  {
    // Setup render target
    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pOutput->m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil));

    // Bind render target and viewport
    auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    auto& globals          = renderViewContext.m_pRenderContext->WriteGlobalConstants();
    globals.NumMsaaSamples = m_MsaaSampleCount;

    renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(nullptr, nullptr, nullptr, xiiGALPrimitiveTopology::TriangleList, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pInput->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    auto pCommandList = renderViewContext.m_pRenderContext->GetCommandList();

    pCommandList->BeginDebugGroup(GetName());
    {
      xiiGALTextureMipLevelData mipLevelData{.m_uiMipLevel = 0U, .m_uiArraySlice = 0U};

      pCommandList->ResolveTextureSubResource(pInput->m_pTexture, mipLevelData, pOutput->m_pTexture, mipLevelData);

      if (renderViewContext.m_pCamera->IsStereoscopic())
      {
        mipLevelData.m_uiArraySlice = 1U;

        pCommandList->ResolveTextureSubResource(pInput->m_pTexture, mipLevelData, pOutput->m_pTexture, mipLevelData);
      }
    }
    pCommandList->EndDebugGroup();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_MsaaResolvePass);
