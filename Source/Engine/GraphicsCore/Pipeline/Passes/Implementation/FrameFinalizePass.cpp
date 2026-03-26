#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/FrameFinalizePass.h>

xiiRenderGraphFrameFinalizePass::xiiRenderGraphFrameFinalizePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("FrameFinalize");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sFrameConstantsResourceName = xiiMakeHashedString("FrameConstants");

  RebuildResourceLayout();
}

void xiiRenderGraphFrameFinalizePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphFrameFinalizePass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphFrameFinalizePass::SetFrameConstantsResourceName(xiiHashedString sResourceName)
{
  m_sFrameConstantsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphFrameFinalizePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphFrameFinalizePass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphFrameFinalizePass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphFrameFinalizePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphFrameFinalizePass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphFrameFinalizePass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphFrameFinalizePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphFrameFinalizePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bEnabled || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  if (m_SetupCommandListFunc.IsValid())
  {
    m_SetupCommandListFunc(*executionContext.m_pCommandList, executionContext);
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

void xiiRenderGraphFrameFinalizePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sFrameConstantsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_FrameFinalizePass);
