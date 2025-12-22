#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/Passes/SimpleRenderPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleRenderPass, 1, xiiRTTIDefaultAllocator<xiiSimpleRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinColour),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_MEMBER_PROPERTY("Message", m_sMessage),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimpleRenderPass::xiiSimpleRenderPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiSimpleRenderPass::~xiiSimpleRenderPass() = default;

xiiResult xiiSimpleRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sMessage;

  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sMessage;

  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  const xiiRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Colour attachment.
  if (pInputs[m_PinColour.m_uiInputIndex])
  {
    pOutputs[m_PinColour.m_uiOutputIndex] = *pInputs[m_PinColour.m_uiInputIndex];
  }
  else if (renderTargets.m_pRTs[0])
  {
    // If no input is available, we use the render target setup instead.

    xiiGALTextureCreationDescription description = renderTargets.m_pRTs[0]->GetTexture()->GetDescription();
    description.m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget);

    pOutputs[m_PinColour.m_uiOutputIndex] = {xiiRenderPipelineNodePinResourceType::ColourAttachment, description};
  }

  // Depth stencil attachment.
  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else if (renderTargets.m_pDSTarget)
  {
    // If no input is available, we use the render target setup instead.

    xiiGALTextureCreationDescription description = renderTargets.m_pDSTarget->GetTexture()->GetDescription();
    description.m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::DepthStencil);

    pOutputs[m_PinColour.m_uiOutputIndex] = {xiiRenderPipelineNodePinResourceType::DepthAttachment, description};
  }

  return XII_SUCCESS;
}

void xiiSimpleRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pColourAttachment = pInputs[m_PinColour.m_uiInputIndex];
  auto pDepthStencil     = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pColourAttachment == nullptr && pDepthStencil == nullptr)
    return;

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({pColourAttachment->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)});
  renderingSetup.SetDepthStencilAttachment({pDepthStencil->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil)});

  auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Setup Permutation Variables.
  xiiTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != xiiViewRenderMode::None)
  {
    sRenderPass = xiiViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }
  pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleTransparent);

  if (!m_sMessage.IsEmpty())
  {
    xiiDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), xiiVec2I32(20, 20), xiiColor::OrangeRed);
  }

  xiiDebugRenderer::RenderWorldSpace(renderViewContext);

  pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::GUI);

  xiiDebugRenderer::RenderScreenSpace(renderViewContext);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SimpleRenderPass);
