#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/SubsurfaceScatteringPass.h>

xiiRenderGraphSubsurfaceScatteringPass::xiiRenderGraphSubsurfaceScatteringPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("SubsurfaceScattering");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sLightingInputResourceName = xiiMakeHashedString("LightingWithAO");
  m_sSssOutputResourceName     = xiiMakeHashedString("SssOutput");

  RebuildResourceLayout();
}

void xiiRenderGraphSubsurfaceScatteringPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphSubsurfaceScatteringPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphSubsurfaceScatteringPass::SetLightingInputResourceName(xiiHashedString sResourceName)
{
  m_sLightingInputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSubsurfaceScatteringPass::SetSssOutputResourceName(xiiHashedString sResourceName)
{
  m_sSssOutputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSubsurfaceScatteringPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphSubsurfaceScatteringPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphSubsurfaceScatteringPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphSubsurfaceScatteringPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphSubsurfaceScatteringPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphSubsurfaceScatteringPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphSubsurfaceScatteringPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLightingInputResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sSssOutputResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_SubsurfaceScatteringPass);
