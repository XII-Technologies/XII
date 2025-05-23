#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpaqueForwardRenderPass, 1, xiiRTTIDefaultAllocator<xiiOpaqueForwardRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    XII_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiOpaqueForwardRenderPass::xiiOpaqueForwardRenderPass(xiiStringView sName) :
  xiiForwardRenderPass(sName)
{
  m_hWhiteTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("White.color");
}

xiiOpaqueForwardRenderPass::~xiiOpaqueForwardRenderPass() = default;

bool xiiOpaqueForwardRenderPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  if (!SUPER::GetRenderTargetDescriptions(view, inputs, outputs))
  {
    return false;
  }

  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    if (inputs[m_PinSSAO.m_uiInputIndex]->m_Size.width != inputs[m_PinColor.m_uiInputIndex]->m_Size.width || inputs[m_PinSSAO.m_uiInputIndex]->m_Size.height != inputs[m_PinColor.m_uiInputIndex]->m_Size.height)
    {
      xiiLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }

    if (m_ShadingQuality == xiiForwardRenderShadingQuality::Simplified)
    {
      xiiLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
    }
  }

  return true;
}

void xiiOpaqueForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(renderViewContext, inputs, outputs);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // SSAO texture
  if (m_ShadingQuality == xiiForwardRenderShadingQuality::Normal)
  {
    if (inputs[m_PinSSAO.m_uiInputIndex])
    {
      xiiSharedPtr<xiiGALTextureView> pSSAOResourceView = inputs[m_PinSSAO.m_uiInputIndex]->m_pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pSSAOResourceView);
    }
    else
    {
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", m_hWhiteTexture, xiiResourceAcquireMode::BlockTillLoaded);
    }
  }
}

void xiiOpaqueForwardRenderPass::SetupPermutationVars(const xiiRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void xiiOpaqueForwardRenderPass::RenderObjects(const xiiRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMasked);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
