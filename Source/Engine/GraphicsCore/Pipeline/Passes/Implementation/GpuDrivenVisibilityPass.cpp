#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>

xiiRenderGraphGpuVisibilityPass::xiiRenderGraphGpuVisibilityPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("GpuVisibilityCulling");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

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

void xiiRenderGraphGpuVisibilityPass::SetDirectDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDirectThreadGroupCountX = uiThreadGroupCountX;
  m_uiDirectThreadGroupCountY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDirectThreadGroupCountZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphGpuVisibilityPass::SetIndirectDispatchArguments(xiiSharedPtr<xiiGALBuffer> pIndirectDispatchArguments, xiiUInt64 uiDispatchArgumentOffset /*= 0U*/, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode /*= xiiGALStateTransitionMode::Transition*/)
{
  m_pIndirectDispatchArguments      = pIndirectDispatchArguments;
  m_uiIndirectDispatchArgumentOffset = uiDispatchArgumentOffset;
  m_IndirectBufferTransitionMode    = bufferTransitionMode;
}

void xiiRenderGraphGpuVisibilityPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphGpuVisibilityPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphGpuVisibilityPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphGpuVisibilityPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphGpuVisibilityPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphGpuVisibilityPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bDispatchEnabled || executionContext.m_pCommandList == nullptr)
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

  if (m_pIndirectDispatchArguments != nullptr)
  {
    executionContext.m_pCommandList->DispatchComputeIndirect(xiiGALDispatchComputeIndirectDescription(m_pIndirectDispatchArguments, m_IndirectBufferTransitionMode, m_uiIndirectDispatchArgumentOffset));

    if (m_PostDispatchCommandListFunc.IsValid())
    {
      m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
    }

    return;
  }

  xiiUInt32 uiThreadGroupCountX = m_uiDirectThreadGroupCountX;
  xiiUInt32 uiThreadGroupCountY = m_uiDirectThreadGroupCountY;
  xiiUInt32 uiThreadGroupCountZ = m_uiDirectThreadGroupCountZ;

  if (uiThreadGroupCountX == 0U)
  {
    if (m_uiInstanceCount == 0U)
    {
      return;
    }

    uiThreadGroupCountX = (m_uiInstanceCount + (m_uiThreadGroupSize - 1U)) / m_uiThreadGroupSize;
    uiThreadGroupCountY = 1U;
    uiThreadGroupCountZ = 1U;
  }

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_GpuDrivenVisibilityPass);
