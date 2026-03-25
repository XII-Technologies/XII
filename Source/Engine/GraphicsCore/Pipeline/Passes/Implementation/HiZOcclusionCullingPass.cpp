#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/HiZOcclusionCullingPass.h>

xiiRenderGraphHiZOcclusionCullingPass::xiiRenderGraphHiZOcclusionCullingPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("HiZOcclusionCulling");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sCandidateInstancesResourceName = xiiMakeHashedString("GpuSceneInstances");
  m_sCandidateInstanceCountResourceName = xiiMakeHashedString("GpuVisibleCandidateCount");
  m_sDepthPyramidResourceName       = xiiMakeHashedString("SceneDepthPyramid");
  m_sVisibleInstancesResourceName   = xiiMakeHashedString("GpuVisibleInstances");
  m_sVisibleInstanceCountResourceName = xiiMakeHashedString("GpuVisibleInstanceCount");

  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphHiZOcclusionCullingPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = uiThreadGroupCountX;
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphHiZOcclusionCullingPass::SetCandidateInstancesResourceName(xiiHashedString sResourceName)
{
  m_sCandidateInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetCandidateInstanceCountResourceName(xiiHashedString sResourceName)
{
  m_sCandidateInstanceCountResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetDepthPyramidResourceName(xiiHashedString sResourceName)
{
  m_sDepthPyramidResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetVisibleInstancesResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetVisibleInstanceCountResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstanceCountResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphHiZOcclusionCullingPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphHiZOcclusionCullingPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphHiZOcclusionCullingPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphHiZOcclusionCullingPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphHiZOcclusionCullingPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphHiZOcclusionCullingPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphHiZOcclusionCullingPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sCandidateInstancesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sDepthPyramidResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sCandidateInstanceCountResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sVisibleInstancesResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sVisibleInstanceCountResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_HiZOcclusionCullingPass);
