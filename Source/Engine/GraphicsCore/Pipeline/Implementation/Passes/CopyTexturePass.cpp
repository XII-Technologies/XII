#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/Pipeline/Passes/CopyTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCopyTexturePass, 1, xiiRTTIDefaultAllocator<xiiCopyTexturePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCopyTexturePass::xiiCopyTexturePass() :
  xiiRenderPipelinePass("CopyTexturePass")
{
}

xiiCopyTexturePass::~xiiCopyTexturePass() = default;

bool xiiCopyTexturePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  const xiiGALTextureCreationDescription* pInput = inputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    xiiGALTextureCreationDescription textureDescription = *pInput;

    outputs[m_PinOutput.m_uiOutputIndex] = textureDescription;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void xiiCopyTexturePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pInput  = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  if (pOutput->m_pTexture->GetDescription().m_Format != pInput->m_pTexture->GetDescription().m_Format)
  {
    /// \todo GraphicsCore: Use a shader when the format is not an exact match.

    xiiLog::Error("Copying textures of different formats is not implemented!");
  }
  else
  {
    if (auto pGraphicsOrTransferQueue = pDevice->GetDefaultCommandQueue(xiiGALCommandQueueType::Transfer))
    {
      auto pCommandList = pGraphicsOrTransferQueue->BeginCommandList();

      pCommandList->BeginDebugGroup(GetName());
      {
        pCommandList->CopyTexture(pInput->m_pTexture, pOutput->m_pTexture);
      }
      pCommandList->EndDebugGroup();
      pCommandList->Submit();
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_CopyTexturePass);
