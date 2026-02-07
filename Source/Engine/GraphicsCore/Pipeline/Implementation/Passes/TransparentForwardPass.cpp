#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/TransparentForwardPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparentForwardRenderPass, 1, xiiRTTIDefaultAllocator<xiiTransparentForwardRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
    XII_MEMBER_PROPERTY("Sampler", m_PinSceneColourSampler),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTransparentForwardRenderPass::xiiTransparentForwardRenderPass(xiiStringView sName) :
  xiiForwardRenderPass(sName)
{
}

xiiTransparentForwardRenderPass::~xiiTransparentForwardRenderPass() = default;

xiiResult xiiTransparentForwardRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_SUCCEED_OR_RETURN(SUPER::GetResourceDescriptions(view, pInputs, pOutputs));

  if (!pInputs[m_PinSceneColourSampler.m_uiInputIndex])
  {
    xiiLog::Error("xiiTransparentForwardRenderPass: Missing input sampler 'SceneColourSampler'.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiTransparentForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  auto pColourInput = pInputs[m_PinColour.m_uiInputIndex];
  if (!pColourInput)
    return;

  auto pSamplerInput = pInputs[m_PinSceneColourSampler.m_uiInputIndex];
  if (!pSamplerInput)
    return;

  xiiGALTextureCreationDescription sceneColourDescription = xiiGALDeviceUtilities::CreateRenderTargetDescription(pColourInput->m_Resource.m_Texture.m_Description.m_Size, pColourInput->m_Resource.m_Texture.m_Description.m_Format);
  sceneColourDescription.m_Type                           = xiiGALResourceDimension::Texture2DArray;
  sceneColourDescription.m_uiArraySizeOrDepth             = pColourInput->m_Resource.m_Texture.m_Description.m_uiArraySizeOrDepth;
  sceneColourDescription.m_uiMipLevels                    = 1U;

  xiiSharedPtr<xiiGALTexture> pSceneColorTexture = xiiGPUResourcePool::GetDefaultInstance()->GetTexture(sceneColourDescription);
  {
    UpdateSceneColorTexture(pSceneColorTexture, pColourInput->m_Resource.m_Texture.m_pTexture);

    SetupResources(renderViewContext, pInputs, pOutputs);
    SetupPermutationVariables(renderViewContext);
    SetupLighting(renderViewContext);

    renderViewContext.m_pRenderContext->BindTexture("SceneColor", pSceneColorTexture);
    renderViewContext.m_pRenderContext->BindSampler("SceneColorSampler", pSamplerInput->m_Resource.m_Sampler.m_pSampler);

    RenderObjects(renderViewContext);

    renderViewContext.m_pRenderContext->EndRendering();
  }
  xiiGPUResourcePool::GetDefaultInstance()->ReturnTexture(std::move(pSceneColorTexture));
}

void xiiTransparentForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  SUPER::SetupResources(renderViewContext, pInputs, pOutputs);

  // Resolved depth texture.
  if (pInputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    renderViewContext.m_pRenderContext->BindTexture("SceneDepth", pInputs[m_PinResolvedDepth.m_uiInputIndex]->m_Resource.m_Texture.m_pTexture);
  }
}

void xiiTransparentForwardRenderPass::RenderObjects(const xiiRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Transparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Foreground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Foreground);

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::ScreenFX);
}

void xiiTransparentForwardRenderPass::UpdateSceneColorTexture(xiiSharedPtr<xiiGALTexture> pSceneColorTexture, xiiSharedPtr<xiiGALTexture> pCurrentColorTexture)
{
  auto pCommandList = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Graphics>("Copy Scene Color Texture");

  pCommandList->CopyTexture(pCurrentColorTexture, pSceneColorTexture);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TransparentForwardPass);
