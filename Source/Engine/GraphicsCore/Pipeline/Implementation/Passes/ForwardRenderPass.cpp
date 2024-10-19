#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Lights/SimplifiedDataProvider.h>
#include <GraphicsCore/Pipeline/Passes/ForwardRenderPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiForwardRenderPass, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_PinColor),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_ENUM_MEMBER_PROPERTY("ShadingQuality", xiiForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new xiiDefaultValueAttribute((int)xiiForwardRenderShadingQuality::Normal)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiForwardRenderShadingQuality, 1)
  XII_ENUM_CONSTANTS(xiiForwardRenderShadingQuality::Normal, xiiForwardRenderShadingQuality::Simplified)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiForwardRenderPass::xiiForwardRenderPass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true), m_ShadingQuality(xiiForwardRenderShadingQuality::Normal)
{
}

xiiForwardRenderPass::~xiiForwardRenderPass() = default;

bool xiiForwardRenderPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No color input connected to pass '{0}'!", GetName());
    return false;
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  SetupResources(renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
}

xiiResult xiiForwardRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_ShadingQuality;
  return XII_SUCCESS;
}

xiiResult xiiForwardRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_ShadingQuality;
  return XII_SUCCESS;
}

void xiiForwardRenderPass::SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetTexture(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
  }

  renderViewContext.m_pRenderContext->BeginRendering(std::move(renderingSetup), renderViewContext.m_pViewData->m_ViewPortRect, "", renderViewContext.m_pCamera->IsStereoscopic());
}

void xiiForwardRenderPass::SetupPermutationVars(const xiiRenderViewContext& renderViewContext)
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

  // Set permutation for shading quality
  if (m_ShadingQuality == xiiForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == xiiForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    XII_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

void xiiForwardRenderPass::SetupLighting(const xiiRenderViewContext& renderViewContext)
{
  // Setup clustered data
  if (m_ShadingQuality == xiiForwardRenderShadingQuality::Normal)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<xiiClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  // Or other light properties.
  else
  {
    auto pSimplifiedData = GetPipeline()->GetFrameDataProvider<xiiSimplifiedDataProvider>()->GetData(renderViewContext);
    pSimplifiedData->BindResources(renderViewContext.m_pRenderContext);
    // todo
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_ForwardRenderPass);
