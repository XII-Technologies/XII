#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CopyTexturePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCopyColourAttachmentPass, 1, xiiRTTIDefaultAllocator<xiiCopyColourAttachmentPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCopyColourAttachmentPass::xiiCopyColourAttachmentPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCopyColourAttachmentPass::~xiiCopyColourAttachmentPass() = default;

xiiResult xiiCopyColourAttachmentPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  const xiiRenderPipelinePassResource* pInput = pInputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    xiiRenderPipelinePassResource resource = *pInput;

    pOutputs[m_PinOutput.m_uiOutputIndex] = resource;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());

    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiCopyColourAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  auto pInput  = pInputs[m_PinInput.m_uiInputIndex];
  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALCommandList> pCommandList = GetPipeline()->CreateCommandListForPass(this);

  {
    xiiGALScopedDebugGroup scope(pCommandList, GetName());

    pCommandList->CopyTexture(pInput->m_Resource.m_Texture.m_pTexture, pOutput->m_Resource.m_Texture.m_pTexture);
  }

  pCommandList->End();

  GetPipeline()->SubmitCommandListForPass(this, std::move(pCommandList));
}

///////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCopyDepthAttachmentPass, 1, xiiRTTIDefaultAllocator<xiiCopyDepthAttachmentPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCopyDepthAttachmentPass::xiiCopyDepthAttachmentPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCopyDepthAttachmentPass::~xiiCopyDepthAttachmentPass() = default;

xiiResult xiiCopyDepthAttachmentPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  const xiiRenderPipelinePassResource* pInput = pInputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    xiiRenderPipelinePassResource resource = *pInput;

    pOutputs[m_PinOutput.m_uiOutputIndex] = resource;
  }
  else
  {
    xiiLog::Error("No input connected to '{0}'!", GetName());

    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiCopyDepthAttachmentPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);

  auto pInput  = pInputs[m_PinInput.m_uiInputIndex];
  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALCommandList> pCommandList = GetPipeline()->CreateCommandListForPass(this);
  XII_ASSERT_DEV(pCommandList != nullptr, "Failed to create command list!");
  pCommandList->Begin();
  {
    xiiGALScopedDebugGroup scope(pCommandList, GetName());

    pCommandList->CopyTexture(pInput->m_Resource.m_Texture.m_pTexture, pOutput->m_Resource.m_Texture.m_pTexture);
  }
  pCommandList->End();

  GetPipeline()->SubmitCommandListForPass(this, std::move(pCommandList));
}
