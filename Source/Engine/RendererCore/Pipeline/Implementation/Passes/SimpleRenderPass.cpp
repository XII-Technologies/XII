#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

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

xiiSimpleRenderPass::~xiiSimpleRenderPass() {}

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
      outputs[m_PinColor.m_uiOutputIndex]                               = pTexture->GetDescription();
      outputs[m_PinColor.m_uiOutputIndex].m_bCreateRenderTarget         = true;
      outputs[m_PinColor.m_uiOutputIndex].m_bAllowShaderResourceView    = true;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bReadBack  = false;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bImmutable = true;
      outputs[m_PinColor.m_uiOutputIndex].m_pExisitingNativeObject      = nullptr;
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
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

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

void xiiSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
