#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/ReflectionDenoisePass.h>

xiiRenderGraphReflectionDenoisePass::xiiRenderGraphReflectionDenoisePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("ReflectionDenoise");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sReflectionInputResourceName  = xiiMakeHashedString("ReflectionColor");
  m_sReflectionOutputResourceName = xiiMakeHashedString("ReflectionDenoised");

  RebuildResourceLayout();
}

void xiiRenderGraphReflectionDenoisePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphReflectionDenoisePass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphReflectionDenoisePass::SetReflectionInputResourceName(xiiHashedString sResourceName)
{
  m_sReflectionInputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphReflectionDenoisePass::SetReflectionOutputResourceName(xiiHashedString sResourceName)
{
  m_sReflectionOutputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphReflectionDenoisePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphReflectionDenoisePass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphReflectionDenoisePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphReflectionDenoisePass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphReflectionDenoisePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphReflectionDenoisePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphReflectionDenoisePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sReflectionInputResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sReflectionOutputResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_ReflectionDenoisePass);
