#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureUtils.h>

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
  if (!m_hSceneColorSampler.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroySampler(m_hSceneColorSampler);
    m_hSceneColorSampler.Invalidate();
  }
}

void xiiTransparentForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
  {
    return;
  }

  CreateSampler();

  xiiGALTextureCreationDescription desc;
  desc.m_Type               = xiiGALResourceDimension::Texture2D;
  desc.m_Size               = pColorInput->m_Desc.m_Size;
  desc.m_uiArraySizeOrDepth = pColorInput->m_Desc.m_uiArraySizeOrDepth;
  desc.m_uiMipLevels        = 1;
  desc.m_uiSampleCount      = 1;
  desc.m_Format             = pColorInput->m_Desc.m_Format;
  desc.m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget);

  xiiGALTextureHandle hSceneColor = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiGALPass*   pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);

  UpdateSceneColorTexture(renderViewContext, hSceneColor, pColorInput->m_TextureHandle);

  xiiGALTextureViewHandle colorResourceViewHandle = pDevice->GetDefaultResourceView(hSceneColor);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", colorResourceViewHandle);
  renderViewContext.m_pRenderContext->BindSampler("SceneColorSampler", m_hSceneColorSampler);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);

  xiiGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hSceneColor);
}

void xiiTransparentForwardRenderPass::SetupResources(xiiGALPass* pGALPass, const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    xiiGALTextureViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", depthResourceViewHandle);
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

void xiiTransparentForwardRenderPass::UpdateSceneColorTexture(const xiiRenderViewContext& renderViewContext, xiiGALTextureHandle hSceneColorTexture, xiiGALTextureHandle hCurrentColorTexture)
{
  xiiGALTextureMipLevelData subresource;
  subresource.m_uiMipLevel   = 0;
  subresource.m_uiArraySlice = 0;

  renderViewContext.m_pRenderContext->GetCommandEncoder()->ResolveTexture(hSceneColorTexture, subresource, hCurrentColorTexture, subresource);
}

void xiiTransparentForwardRenderPass::CreateSampler()
{
  if (m_hSceneColorSampler.IsInvalidated())
  {
    xiiGALSamplerCreationDescription desc;
    desc.m_MinFilter = xiiGALFilterType::Linear;
    desc.m_MagFilter = xiiGALFilterType::Linear;
    desc.m_MipFilter = xiiGALFilterType::Linear;
    desc.m_AddressU  = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressV  = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Mirror);
    desc.m_AddressW  = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Mirror);

    m_hSceneColorSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(desc);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
