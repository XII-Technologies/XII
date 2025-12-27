#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/OpaqueForwardPass.h>
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

xiiResult xiiOpaqueForwardRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_bWriteDepth;

  return XII_SUCCESS;
}

xiiResult xiiOpaqueForwardRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_bWriteDepth;

  return XII_SUCCESS;
}

xiiResult xiiOpaqueForwardRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_SUCCEED_OR_RETURN(SUPER::GetResourceDescriptions(view, pInputs, pOutputs));

  if (pInputs[m_PinSSAO.m_uiInputIndex])
  {
    if (pInputs[m_PinSSAO.m_uiInputIndex]->m_Texture.m_Description.GetWidth() != pInputs[m_PinColour.m_uiInputIndex]->m_Texture.m_Description.GetWidth() || pInputs[m_PinSSAO.m_uiInputIndex]->m_Texture.m_Description.GetHeight() != pInputs[m_PinColour.m_uiInputIndex]->m_Texture.m_Description.GetHeight())
    {
      xiiLog::Warning("SSAO input texture size does not match the colour attachment size.");
    }
    if (m_ShadingQuality < xiiForwardRenderShadingQuality::Medium)
    {
      xiiLog::Warning("SSAO input will be ignored, as shading quality is set to '{0}'. SSAO requires at least 'Medium' quality.", m_ShadingQuality.GetValue());
    }
  }
  return XII_SUCCESS;
}

void xiiOpaqueForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  SUPER::SetupResources(renderViewContext, pInputs, pOutputs);

  // SSAO input texture.
  if (pInputs[m_PinSSAO.m_uiInputIndex] && m_ShadingQuality >= xiiForwardRenderShadingQuality::Medium)
  {
    renderViewContext.m_pRenderContext->BindTexture("SSAOTexture", pInputs[m_PinSSAO.m_uiInputIndex]->m_Resource.m_Texture.m_pTexture);
  }
  else
  {
    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", m_hWhiteTexture, xiiResourceAcquireMode::BlockTillLoaded);
  }
}

void xiiOpaqueForwardRenderPass::SetupPermutationVariables(const xiiRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVariables(renderViewContext);

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
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaqueStatic);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaqueDynamic);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMaskedStatic);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMaskedDynamic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_OpaqueForwardPass);
