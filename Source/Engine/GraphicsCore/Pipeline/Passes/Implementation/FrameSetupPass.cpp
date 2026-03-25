#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>

#include <Foundation/Math/Color.h>

xiiRenderGraphFrameSetupPass::xiiRenderGraphFrameSetupPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("FrameSetup");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sPreviousFrameStatsResourceName   = xiiMakeHashedString("PreviousFrameStats");
  m_sFrameConstantsResourceName       = xiiMakeHashedString("FrameConstants");
  m_sFrameTimingResourceName          = xiiMakeHashedString("FrameTimingData");
  m_sFrameTimestampRangesResourceName = xiiMakeHashedString("FrameTimestampRanges");

  RebuildResourceLayout();
}

void xiiRenderGraphFrameSetupPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphFrameSetupPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphFrameSetupPass::SetPreviousFrameStatsResourceName(xiiHashedString sResourceName)
{
  m_sPreviousFrameStatsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphFrameSetupPass::SetFrameConstantsResourceName(xiiHashedString sResourceName)
{
  m_sFrameConstantsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphFrameSetupPass::SetFrameTimingResourceName(xiiHashedString sResourceName)
{
  m_sFrameTimingResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphFrameSetupPass::SetFrameTimestampRangesResourceName(xiiHashedString sResourceName)
{
  m_sFrameTimestampRangesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphFrameSetupPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphFrameSetupPass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphFrameSetupPass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphFrameSetupPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphFrameSetupPass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphFrameSetupPass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphFrameSetupPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphFrameSetupPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bEnabled || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  executionContext.m_pCommandList->BeginDebugGroup("FrameSetup", xiiColor::SteelBlue);

  if (m_SetupCommandListFunc.IsValid())
  {
    m_SetupCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (executionContext.m_pCommandList->CommitShaderResources().Failed())
  {
    executionContext.m_pCommandList->EndDebugGroup();
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

  executionContext.m_pCommandList->EndDebugGroup();
}

void xiiRenderGraphFrameSetupPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sPreviousFrameStatsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sFrameConstantsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sFrameTimingResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sFrameTimestampRangesResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_FrameSetupPass);
