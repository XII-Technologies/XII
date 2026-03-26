#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/PresentCopyPass.h>

xiiRenderGraphPresentCopyPass::xiiRenderGraphPresentCopyPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("PresentCopy");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sFinalColorResourceName    = xiiMakeHashedString("DebugOverlayOutput");
  m_sPresentTargetResourceName = xiiMakeHashedString("PresentTarget");

  RebuildResourceLayout();
}

void xiiRenderGraphPresentCopyPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphPresentCopyPass::SetFinalColorResourceName(xiiHashedString sResourceName)
{
  m_sFinalColorResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphPresentCopyPass::SetPresentTargetResourceName(xiiHashedString sResourceName)
{
  m_sPresentTargetResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphPresentCopyPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphPresentCopyPass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphPresentCopyPass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphPresentCopyPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphPresentCopyPass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphPresentCopyPass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphPresentCopyPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphPresentCopyPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bEnabled || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  if (m_SetupCommandListFunc.IsValid())
  {
    m_SetupCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (executionContext.m_pCommandList->CommitShaderResources().Failed())
  {
    return;
  }

  if (m_ExecuteCommandListFunc.IsValid())
  {
    m_ExecuteCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (m_PostExecuteCommandListFunc.IsValid())
  {
    m_PostExecuteCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphPresentCopyPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sFinalColorResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sPresentTargetResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::ReadWrite | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_PresentCopyPass);
