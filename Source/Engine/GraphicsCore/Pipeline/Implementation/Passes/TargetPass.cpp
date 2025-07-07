#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/TargetPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTargetPass, 1, xiiRTTIDefaultAllocator<xiiTargetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Color0", m_PinColor0),
    XII_MEMBER_PROPERTY("Color1", m_PinColor1),
    XII_MEMBER_PROPERTY("Color2", m_PinColor2),
    XII_MEMBER_PROPERTY("Color3", m_PinColor3),
    XII_MEMBER_PROPERTY("Color4", m_PinColor4),
    XII_MEMBER_PROPERTY("Color5", m_PinColor5),
    XII_MEMBER_PROPERTY("Color6", m_PinColor6),
    XII_MEMBER_PROPERTY("Color7", m_PinColor7),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTargetPass::xiiTargetPass(xiiStringView sName) :
  xiiPresentPipelinePass(sName)
{
}

xiiTargetPass::~xiiTargetPass() = default;

xiiResult xiiTargetPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  m_pSwapChain    = view.GetSwapChain();
  m_RenderTargets = view.GetRenderTargets();
  
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiTargetPass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  XII_ASSERT_DEV(pPin->m_pParent == this, "xiiTargetPass::QueryTextureProvider: The given pin is not part of this pass!");

  if (pPin->m_uiOutputIndex == 8)
  {
    return m_RenderTargets.m_pDSTarget;
  }
  else
  {
    if (m_pSwapChain)
    {
      if (pPin->m_uiInputIndex == 0)
      {
        return m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget);
      }
    }
    else
    {
      return m_RenderTargets.m_pRTs[pPin->m_uiInputIndex];
    }
  }
  return xiiSharedPtr<xiiGALDeviceObject>();
}

void xiiTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TargetPass);
