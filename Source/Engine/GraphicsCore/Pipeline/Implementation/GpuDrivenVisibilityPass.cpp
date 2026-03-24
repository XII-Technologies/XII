#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/GpuDrivenVisibilityPass.h>

xiiRenderGraphGpuVisibilityPass::xiiRenderGraphGpuVisibilityPass()
{
  m_PassDescription.m_sPassName        = xiiMakeHashedString("GpuVisibilityCulling");
  m_PassDescription.m_QueueFlags       = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects  = false;

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = xiiMakeHashedString("GpuSceneInstances");
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = xiiMakeHashedString("GpuVisibleInstances");
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = xiiMakeHashedString("GpuVisibleInstanceCount");
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

void xiiRenderGraphGpuVisibilityPass::SetInstanceCount(xiiUInt32 uiInstanceCount)
{
  m_uiInstanceCount = uiInstanceCount;
}

void xiiRenderGraphGpuVisibilityPass::SetDispatchEnabled(bool bDispatchEnabled)
{
  m_bDispatchEnabled = bDispatchEnabled;
}

void xiiRenderGraphGpuVisibilityPass::SetThreadGroupSize(xiiUInt32 uiThreadGroupSize)
{
  m_uiThreadGroupSize = xiiMath::Max(1U, uiThreadGroupSize);
}

const xiiRenderGraphPassDescription& xiiRenderGraphGpuVisibilityPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphGpuVisibilityPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bDispatchEnabled || m_uiInstanceCount == 0U || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  const xiiUInt32 uiThreadGroupCountX = (m_uiInstanceCount + (m_uiThreadGroupSize - 1U)) / m_uiThreadGroupSize;
  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(uiThreadGroupCountX, 1U, 1U));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_GpuDrivenVisibilityPass);
