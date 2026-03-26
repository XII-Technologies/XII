#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/SsaoCompositePass.h>

xiiRenderGraphSsaoCompositePass::xiiRenderGraphSsaoCompositePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("SsaoComposite");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sLightingInputResourceName  = xiiMakeHashedString("LightingBuffer");
  m_sSsaoFilteredResourceName   = xiiMakeHashedString("SsaoFiltered");
  m_sLightingOutputResourceName = xiiMakeHashedString("LightingWithAO");

  RebuildResourceLayout();
}

void xiiRenderGraphSsaoCompositePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphSsaoCompositePass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphSsaoCompositePass::SetLightingInputResourceName(xiiHashedString sResourceName)
{
  m_sLightingInputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSsaoCompositePass::SetSsaoFilteredResourceName(xiiHashedString sResourceName)
{
  m_sSsaoFilteredResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSsaoCompositePass::SetLightingOutputResourceName(xiiHashedString sResourceName)
{
  m_sLightingOutputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSsaoCompositePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphSsaoCompositePass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphSsaoCompositePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphSsaoCompositePass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphSsaoCompositePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphSsaoCompositePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphSsaoCompositePass::RebuildResourceLayout()
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
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sSsaoFilteredResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLightingOutputResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_SsaoCompositePass);
