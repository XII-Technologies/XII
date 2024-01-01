#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/SimpleRenderPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleRenderPass, 1, xiiRTTIDefaultAllocator<xiiSimpleRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color", m_PinColor),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("Message", m_sMessage),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimpleRenderPass::xiiSimpleRenderPass(const char* szName) :
  xiiRenderPipelinePass(szName, true)
{
}

xiiSimpleRenderPass::~xiiSimpleRenderPass() = default;

bool xiiSimpleRenderPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  xiiGALDevice*              pDevice       = xiiGALDevice::GetDefaultDevice();
  const xiiGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const xiiGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
    if (pTexture)
    {
      outputs[m_PinColor.m_uiOutputIndex] = pTexture->GetDescription();
      outputs[m_PinColor.m_uiOutputIndex].m_BindFlags.Add(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget);
    }
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const xiiGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hDSTarget);
    if (pTexture)
    {
      outputs[m_PinDepthStencil.m_uiOutputIndex] = pTexture->GetDescription();
    }
  }

  return true;
}

void xiiSimpleRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
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

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Execute update functions
  UpdateDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleOpaque);
  UpdateDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleTransparent);
  UpdateDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);
  UpdateDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::GUI);

  // Setup Permutation Vars
  xiiTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != xiiViewRenderMode::None)
  {
    sRenderPass = xiiViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  // Execute render functions
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleTransparent);

  if (!m_sMessage.IsEmpty())
  {
    xiiDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), xiiVec2I32(20, 20), xiiColor::OrangeRed);
  }

  xiiDebugRenderer::Render(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::GUI);
}

xiiResult xiiSimpleRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sMessage;
  return XII_SUCCESS;
}

xiiResult xiiSimpleRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sMessage;
  return XII_SUCCESS;
}

void xiiSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SimpleRenderPass);
