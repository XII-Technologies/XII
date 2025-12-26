#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Lights/SimplifiedDataProvider.h>
#include <GraphicsCore/Pipeline/Passes/ForwardPass.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiForwardRenderShadingQuality, 1)
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Low),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Medium),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::High),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Ultra)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiForwardRenderPass, 3, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinColour),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_ENUM_MEMBER_PROPERTY("ShadingQuality", xiiForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new xiiDefaultValueAttribute(xiiForwardRenderShadingQuality::Medium)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiForwardRenderPass::xiiForwardRenderPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiForwardRenderPass::~xiiForwardRenderPass() = default;

xiiResult xiiForwardRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_ShadingQuality;

  return XII_SUCCESS;
}

xiiResult xiiForwardRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_ShadingQuality;

  return XII_SUCCESS;
}

xiiResult xiiForwardRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinColour.m_uiInputIndex])
  {
    pOutputs[m_PinColour.m_uiOutputIndex] = *pInputs[m_PinColour.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No colour attachment input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  // Depth stencil attachment.
  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  SetupResources(renderViewContext, pInputs, pOutputs);

  SetupPermutationVariables(renderViewContext);
  SetupLighting(renderViewContext);
  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
}

void xiiForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiRenderingSetup renderingSetup;

  if (inputs[m_PinColour.m_uiInputIndex])
  {
    renderingSetup.AddColorAttachment({inputs[m_PinColour.m_uiInputIndex]->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget)});
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.SetDepthStencilAttachment({inputs[m_PinDepthStencil.m_uiInputIndex]->m_Resource.m_Texture.m_pTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil)});
  }

  renderingSetup.Build();

  renderViewContext.m_pRenderContext->BeginRendering(renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, GetName(), renderViewContext.m_pCamera->IsStereoscopic());
}

void xiiForwardRenderPass::SetupPermutationVariables(const xiiRenderViewContext& renderViewContext)
{
  xiiTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != xiiViewRenderMode::None)
  {
    sRenderPass = xiiViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  xiiStringBuilder sDebugText;
  xiiViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    xiiDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, xiiVec2I32(10, 10), xiiColor::White);
  }

  // Set permutation for shading quality.
  if (m_ShadingQuality == xiiForwardRenderShadingQuality::Low)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_LOW");
  }
  else if (m_ShadingQuality == xiiForwardRenderShadingQuality::Medium)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_MEDIUM");
  }
  else if (m_ShadingQuality == xiiForwardRenderShadingQuality::High)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_HIGH");
  }
  else if (m_ShadingQuality == xiiForwardRenderShadingQuality::Ultra)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_ULTRA");
  }
  else
  {
    XII_REPORT_FAILURE("Unknown shading quality setting!");
  }
}

void xiiForwardRenderPass::SetupLighting(const xiiRenderViewContext& renderViewContext)
{
  // Setup lighting data here (e.g., upload light buffers, set shader parameters).

  // Setup clustered data.
  if (m_ShadingQuality >= xiiShadingQualityLevel::Medium)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<xiiClusteredDataProvider>()->GetData(renderViewContext);

    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  else
  {
    auto pSimplifiedData = GetPipeline()->GetFrameDataProvider<xiiSimplifiedDataProvider>()->GetData(renderViewContext);

    pSimplifiedData->BindResources(renderViewContext.m_pRenderContext);
  }
}
