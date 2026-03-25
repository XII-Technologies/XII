#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DrawCommandBuildPass.h>

xiiRenderGraphDrawCommandBuildPass::xiiRenderGraphDrawCommandBuildPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("DrawCommandBuild");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sVisibleInstancesResourceName      = xiiMakeHashedString("GpuVisibleInstances");
  m_sLodSelectionResourceName          = xiiMakeHashedString("GpuLodSelections");
  m_sIndirectCommandBufferResourceName = xiiMakeHashedString("GpuIndirectDrawCommands");
  m_sIndirectCountBufferResourceName   = xiiMakeHashedString("GpuIndirectDrawCounts");

  RebuildResourceLayout();
}

void xiiRenderGraphDrawCommandBuildPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphDrawCommandBuildPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = uiThreadGroupCountX;
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphDrawCommandBuildPass::SetVisibleInstancesResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDrawCommandBuildPass::SetLodSelectionResourceName(xiiHashedString sResourceName)
{
  m_sLodSelectionResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDrawCommandBuildPass::SetIndirectCommandBufferResourceName(xiiHashedString sResourceName)
{
  m_sIndirectCommandBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDrawCommandBuildPass::SetIndirectCountBufferResourceName(xiiHashedString sResourceName)
{
  m_sIndirectCountBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDrawCommandBuildPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphDrawCommandBuildPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphDrawCommandBuildPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphDrawCommandBuildPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphDrawCommandBuildPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphDrawCommandBuildPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_uiDispatchThreadGroupsX == 0U)
  {
    return;
  }

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(m_uiDispatchThreadGroupsX, m_uiDispatchThreadGroupsY, m_uiDispatchThreadGroupsZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphDrawCommandBuildPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sVisibleInstancesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLodSelectionResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sIndirectCommandBufferResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sIndirectCountBufferResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_DrawCommandBuildPass);
