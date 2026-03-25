#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/CoarseFrustumCullingPass.h>

xiiRenderGraphCoarseFrustumCullingPass::xiiRenderGraphCoarseFrustumCullingPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("CoarseFrustumCulling");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sSceneBoundsResourceName         = xiiMakeHashedString("GpuSceneBounds");
  m_sCameraFrustumPlanesResourceName = xiiMakeHashedString("CameraFrustumPlanes");
  m_sVisibleInstancesResourceName    = xiiMakeHashedString("GpuVisibleInstances");
  m_sVisibleInstanceCountResourceName = xiiMakeHashedString("GpuVisibleInstanceCount");

  RebuildResourceLayout();
}

void xiiRenderGraphCoarseFrustumCullingPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphCoarseFrustumCullingPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = uiThreadGroupCountX;
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphCoarseFrustumCullingPass::SetSceneBoundsResourceName(xiiHashedString sResourceName)
{
  m_sSceneBoundsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphCoarseFrustumCullingPass::SetCameraFrustumPlanesResourceName(xiiHashedString sResourceName)
{
  m_sCameraFrustumPlanesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphCoarseFrustumCullingPass::SetVisibleInstancesResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphCoarseFrustumCullingPass::SetVisibleInstanceCountResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstanceCountResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphCoarseFrustumCullingPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphCoarseFrustumCullingPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphCoarseFrustumCullingPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphCoarseFrustumCullingPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphCoarseFrustumCullingPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphCoarseFrustumCullingPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphCoarseFrustumCullingPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sSceneBoundsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sCameraFrustumPlanesResourceName;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_CoarseFrustumCullingPass);
