#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

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

xiiTransparentForwardRenderPass::xiiTransparentForwardRenderPass(const char* szName) :
  xiiForwardRenderPass(szName)
{
}

xiiTransparentForwardRenderPass::~xiiTransparentForwardRenderPass()
{
  if (!m_hSceneColorSamplerState.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
    m_hSceneColorSamplerState.Invalidate();
  }
}

void xiiTransparentForwardRenderPass::Execute(const xiiRenderViewContext&                               renderViewContext,
                                              const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
                                              const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
  {
    return;
  }

  CreateSamplerState();

  xiiUInt32 uiWidth  = pColorInput->m_Desc.m_uiWidth;
  xiiUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;

  xiiGALTextureCreationDescription desc;
  desc.SetAsRenderTarget(uiWidth, uiHeight, pColorInput->m_Desc.m_Format);
  desc.m_uiArraySize     = pColorInput->m_Desc.m_uiArraySize;
  desc.m_uiMipLevelCount = 1;

  xiiGALTextureHandle hSceneColor = xiiGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiGALPass*   pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  UpdateSceneColorTexture(renderViewContext, hSceneColor, pColorInput->m_TextureHandle);

  xiiGALResourceViewHandle colorResourceViewHandle = pDevice->GetDefaultResourceView(hSceneColor);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", colorResourceViewHandle);
  renderViewContext.m_pRenderContext->BindSamplerState("SceneColorSampler", m_hSceneColorSamplerState);

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
    xiiGALResourceViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
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
}

void xiiTransparentForwardRenderPass::UpdateSceneColorTexture(const xiiRenderViewContext& renderViewContext, xiiGALTextureHandle hSceneColorTexture, xiiGALTextureHandle hCurrentColorTexture)
{
  xiiGALTextureSubresource subresource;
  subresource.m_uiMipLevel   = 0;
  subresource.m_uiArraySlice = 0;

  renderViewContext.m_pRenderContext->GetCommandEncoder()->ResolveTexture(hSceneColorTexture, subresource, hCurrentColorTexture, subresource);
}

void xiiTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    xiiGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = xiiGALTextureFilterMode::Linear;
    desc.m_MagFilter = xiiGALTextureFilterMode::Linear;
    desc.m_MipFilter = xiiGALTextureFilterMode::Linear;
    desc.m_AddressU  = xiiImageAddressMode::Clamp;
    desc.m_AddressV  = xiiImageAddressMode::Mirror;
    desc.m_AddressW  = xiiImageAddressMode::Mirror;

    m_hSceneColorSamplerState = xiiGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
