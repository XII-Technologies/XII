#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/ShadowCasterCullingPass.h>

xiiRenderGraphShadowCasterCullingPass::xiiRenderGraphShadowCasterCullingPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("ShadowCasterCulling");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sInstanceDataResourceName       = xiiMakeHashedString("GpuSceneBounds");
  m_sShadowCascadeDataResourceName  = xiiMakeHashedString("ShadowCascadeData");
  m_sShadowVisibleListResourceName  = xiiMakeHashedString("ShadowVisibleList");
  m_sShadowVisibleCountResourceName = xiiMakeHashedString("ShadowVisibleCount");

  RebuildResourceLayout();
}

void xiiRenderGraphShadowCasterCullingPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphShadowCasterCullingPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphShadowCasterCullingPass::SetInstanceDataResourceName(xiiHashedString sResourceName)
{
  m_sInstanceDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphShadowCasterCullingPass::SetShadowCascadeDataResourceName(xiiHashedString sResourceName)
{
  m_sShadowCascadeDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphShadowCasterCullingPass::SetShadowVisibleListResourceName(xiiHashedString sResourceName)
{
  m_sShadowVisibleListResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphShadowCasterCullingPass::SetShadowVisibleCountResourceName(xiiHashedString sResourceName)
{
  m_sShadowVisibleCountResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphShadowCasterCullingPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphShadowCasterCullingPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphShadowCasterCullingPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphShadowCasterCullingPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphShadowCasterCullingPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphShadowCasterCullingPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphShadowCasterCullingPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sInstanceDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sShadowCascadeDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sShadowVisibleListResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sShadowVisibleCountResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_ShadowCasterCullingPass);
