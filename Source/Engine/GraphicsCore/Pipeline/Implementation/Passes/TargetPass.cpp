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
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Output")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTargetPass::xiiTargetPass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, xiiRenderPipelinePassFlags::StereoAware, xiiRenderPipelinePassConcurrencyHint::Sequential)
{
}

xiiTargetPass::~xiiTargetPass() = default;

bool xiiTargetPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  m_pSwapChain    = view.GetSwapChain();
  m_RenderTargets = view.GetRenderTargets();

  const char* pinNames[] = {
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "Color4",
    "Color5",
    "Color6",
    "Color7",
    "DepthStencil",
  };

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(pinNames); ++i)
  {
    if (!VerifyInput(view, inputs, pinNames[i]))
      return false;
  }

  return true;
}

xiiSharedPtr<xiiGALTextureView> xiiTargetPass::QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc)
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
  return xiiSharedPtr<xiiGALTextureView>();
}

void xiiTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
}

bool xiiTargetPass::VerifyInput(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiStringView sPinName)
{
  const xiiRenderPipelineNodePin* pPin = GetPinByName(sPinName);

  if (inputs[pPin->m_uiInputIndex])
  {
    if (xiiSharedPtr<xiiGALTextureView> pTextureView = QueryTextureProvider(pPin, *inputs[pPin->m_uiInputIndex]))
    {
      if (pTextureView)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTextureView->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return true;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_TargetPass);
