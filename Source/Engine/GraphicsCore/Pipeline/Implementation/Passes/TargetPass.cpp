#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/TargetPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTargetPass, 1, xiiRTTIDefaultAllocator<xiiTargetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour0", m_PinColour0),
    XII_MEMBER_PROPERTY("Colour1", m_PinColour1),
    XII_MEMBER_PROPERTY("Colour2", m_PinColour2),
    XII_MEMBER_PROPERTY("Colour3", m_PinColour3),
    XII_MEMBER_PROPERTY("Colour4", m_PinColour4),
    XII_MEMBER_PROPERTY("Colour5", m_PinColour5),
    XII_MEMBER_PROPERTY("Colour6", m_PinColour6),
    XII_MEMBER_PROPERTY("Colour7", m_PinColour7),
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

xiiResult xiiTargetPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);

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
        return m_pSwapChain->GetBackBufferTexture();
      }
    }
    else
    {
      if (m_RenderTargets.m_pRTs[pPin->m_uiInputIndex])
      {
        return m_RenderTargets.m_pRTs[pPin->m_uiInputIndex]->GetTexture();
      }
    }
  }
  return xiiSharedPtr<xiiGALDeviceObject>();
}

void xiiTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TargetPass);
