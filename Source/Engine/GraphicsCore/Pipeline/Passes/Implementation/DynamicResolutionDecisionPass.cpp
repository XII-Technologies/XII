#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DynamicResolutionDecisionPass.h>

xiiRenderGraphDynamicResolutionDecisionPass::xiiRenderGraphDynamicResolutionDecisionPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("DynamicResolutionDecision");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sFrameTimingResourceName              = xiiMakeHashedString("FrameTimingData");
  m_sDynamicResolutionDecisionResourceName = xiiMakeHashedString("DynamicResolutionData");

  RebuildResourceLayout();
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetFrameTimingResourceName(xiiHashedString sResourceName)
{
  m_sFrameTimingResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetDynamicResolutionDecisionResourceName(xiiHashedString sResourceName)
{
  m_sDynamicResolutionDecisionResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphDynamicResolutionDecisionPass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphDynamicResolutionDecisionPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphDynamicResolutionDecisionPass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphDynamicResolutionDecisionPass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphDynamicResolutionDecisionPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphDynamicResolutionDecisionPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphDynamicResolutionDecisionPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sFrameTimingResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sDynamicResolutionDecisionResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_DynamicResolutionDecisionPass);
