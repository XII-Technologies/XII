#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/LightListBuildPass.h>

xiiRenderGraphLightListBuildPass::xiiRenderGraphLightListBuildPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("LightListBuild");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sVisibleLightsResourceName          = xiiMakeHashedString("VisibleLightList");
  m_sClusterDescriptorsResourceName     = xiiMakeHashedString("ClusterDescriptors");
  m_sClusterDepthInfoResourceName       = xiiMakeHashedString("ClusterDepthRange");
  m_sClusterLightIndicesResourceName    = xiiMakeHashedString("ClusterLightIndices");
  m_sClusterLightPrefixSumsResourceName = xiiMakeHashedString("ClusterLightPrefixSums");

  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphLightListBuildPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphLightListBuildPass::SetVisibleLightsResourceName(xiiHashedString sResourceName)
{
  m_sVisibleLightsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetClusterGridResourceName(xiiHashedString sResourceName)
{
  m_sClusterDescriptorsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetClusterDepthInfoResourceName(xiiHashedString sResourceName)
{
  m_sClusterDepthInfoResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetClusterLightListResourceName(xiiHashedString sResourceName)
{
  m_sClusterLightIndicesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetClusterLightIndicesResourceName(xiiHashedString sResourceName)
{
  m_sClusterLightIndicesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetClusterLightPrefixSumsResourceName(xiiHashedString sResourceName)
{
  m_sClusterLightPrefixSumsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightListBuildPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphLightListBuildPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphLightListBuildPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphLightListBuildPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphLightListBuildPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphLightListBuildPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(m_uiDispatchThreadGroupsX, m_uiDispatchThreadGroupsY, m_uiDispatchThreadGroupsZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphLightListBuildPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sVisibleLightsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sClusterDescriptorsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sClusterDepthInfoResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sClusterLightIndicesResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sClusterLightPrefixSumsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_LightListBuildPass);
