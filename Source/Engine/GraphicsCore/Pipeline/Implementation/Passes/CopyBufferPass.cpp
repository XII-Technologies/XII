#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CopyBufferPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCopyBufferPass, 1, xiiRTTIDefaultAllocator<xiiCopyBufferPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCopyBufferPass::xiiCopyBufferPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiCopyBufferPass::~xiiCopyBufferPass() = default;

xiiResult xiiCopyBufferPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
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

void xiiCopyBufferPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  auto pInput  = pInputs[m_PinInput.m_uiInputIndex];
  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

  xiiSharedPtr<xiiGALCommandList> pCommandList = GetPipeline()->CreateCommandListForPass(this);
  XII_ASSERT_DEV(pCommandList != nullptr, "Failed to create command list!");

  {
    xiiGALScopedDebugGroup scope(pCommandList, GetName());

    pCommandList->CopyBuffer(pInput->m_Resource.m_Buffer.m_pBuffer, pOutput->m_Resource.m_Buffer.m_pBuffer);
  }

  pCommandList->End();

  GetPipeline()->SubmitCommandListForPass(this, std::move(pCommandList));
}
