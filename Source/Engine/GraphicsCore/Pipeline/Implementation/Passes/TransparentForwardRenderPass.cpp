#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/TransparentForwardRenderPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparentForwardRenderPass, 1, xiiRTTIDefaultAllocator<xiiTransparentForwardRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTransparentForwardRenderPass::xiiTransparentForwardRenderPass(xiiStringView sName) :
  xiiForwardRenderPass(sName)
{
}

xiiTransparentForwardRenderPass::~xiiTransparentForwardRenderPass()
{
  m_pSceneColorSampler.Clear();
}

void xiiTransparentForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
    return;

  CreateSampler();

  xiiGALTextureCreationDescription desc = xiiGALDeviceUtilities::CreateRenderTargetDescription(pColorInput->m_TextureDescription.m_Size, pColorInput->m_TextureDescription.m_Format);
  desc.m_uiArraySizeOrDepth             = pColorInput->m_TextureDescription.GetArraySize();

  xiiSharedPtr<xiiGALTexture> pSceneColor = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  {
    SetupResources(renderViewContext, inputs, outputs);
    SetupPermutationVars(renderViewContext);
    SetupLighting(renderViewContext);

    UpdateSceneColorTexture(renderViewContext, pSceneColor, pColorInput->m_pTexture);

    xiiSharedPtr<xiiGALTextureView> pColorTextureView = pSceneColor->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", pColorTextureView);
    renderViewContext.m_pRenderContext->BindSampler("SceneColorSampler", m_pSceneColorSampler);

    RenderObjects(renderViewContext);

    renderViewContext.m_pRenderContext->EndRendering();
  }
  xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(pSceneColor);
}

void xiiTransparentForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(renderViewContext, inputs, outputs);

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    xiiSharedPtr<xiiGALTextureView> pDepthTextureView = inputs[m_PinResolvedDepth.m_uiInputIndex]->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);

    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", pDepthTextureView);
  }
}

void xiiTransparentForwardRenderPass::RenderObjects(const xiiRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitForeground);

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitScreenFX);
}

void xiiTransparentForwardRenderPass::UpdateSceneColorTexture(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALTexture> pSceneColorTexture, xiiSharedPtr<xiiGALTexture> pCurrentColorTexture)
{
  const xiiGALTextureCreationDescription& textureDescription = pCurrentColorTexture->GetDescription();

  if (textureDescription.m_uiSampleCount > xiiGALMSAASampleCount::OneSample)
  {
    xiiGALTextureMipLevelData subresource;
    subresource.m_uiMipLevel   = 0;
    subresource.m_uiArraySlice = 0;

    renderViewContext.m_pRenderContext->GetCommandList()->ResolveTextureSubResource(pCurrentColorTexture, subresource, pSceneColorTexture, subresource);
  }
  else
  {
    renderViewContext.m_pRenderContext->GetCommandList()->CopyTexture(pCurrentColorTexture, pSceneColorTexture);
  }
}

void xiiTransparentForwardRenderPass::CreateSampler()
{
  if (!m_pSceneColorSampler)
  {
    xiiGALSamplerCreationDescription samplerDescription;
    samplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Mirror);
    samplerDescription.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Mirror);
    samplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
    samplerDescription.m_BorderColor        = xiiColor::Black;
    samplerDescription.m_fMipLODBias        = 0.0f;
    samplerDescription.m_fMinLOD            = -1.0f;
    samplerDescription.m_fMaxLOD            = 42000.0f;
    samplerDescription.m_uiMaxAnisotropy    = 4U;

    m_pSceneColorSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(samplerDescription);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
