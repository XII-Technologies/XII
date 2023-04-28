#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

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

xiiTargetPass::xiiTargetPass(const char* szName) :
  xiiRenderPipelinePass(szName, true)
{
}

xiiTargetPass::~xiiTargetPass() {}

const xiiGALTextureHandle* xiiTargetPass::GetTextureHandle(const xiiGALRenderTargets& renderTargets, const xiiRenderPipelineNodePin* pPin)
{
  // auto inputs = GetInputPins();
  if (pPin->m_pParent != this)
  {
    xiiLog::Error("xiiTargetPass::GetTextureHandle: The given pin is not part of this pass!");
    return nullptr;
  }

  xiiGALTextureHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    return &renderTargets.m_hDSTarget;
  }
  else
  {
    return &renderTargets.m_hRTs[pPin->m_uiInputIndex];
  }

  return nullptr;
}

bool xiiTargetPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
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

void xiiTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) {}

bool xiiTargetPass::VerifyInput(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, const char* szPinName)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiRenderPipelineNodePin* pPin = GetPinByName(szPinName);
  if (inputs[pPin->m_uiInputIndex])
  {
    const xiiGALTextureHandle* pHandle = GetTextureHandle(view.GetActiveRenderTargets(), pPin);
    if (pHandle)
    {
      const xiiGALTexture* pTexture = pDevice->GetTexture(*pHandle);
      if (pTexture)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return true;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TargetPass);
