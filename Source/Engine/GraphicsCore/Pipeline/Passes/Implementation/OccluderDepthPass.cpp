#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/OccluderDepthPass.h>

xiiRenderGraphOccluderDepthPass::xiiRenderGraphOccluderDepthPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("OccluderDepth");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sOccluderInstancesResourceName = xiiMakeHashedString("OccluderInstances");
  m_sOccluderDepthResourceName     = xiiMakeHashedString("OccluderDepth");

  RebuildResourceLayout();
}

void xiiRenderGraphOccluderDepthPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphOccluderDepthPass::SetOccluderInstancesResourceName(xiiHashedString sResourceName)
{
  m_sOccluderInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphOccluderDepthPass::SetOccluderDepthResourceName(xiiHashedString sResourceName)
{
  m_sOccluderDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphOccluderDepthPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphOccluderDepthPass::SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc)
{
  m_DrawCommandListFunc = drawCommandListFunc;
}

void xiiRenderGraphOccluderDepthPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphOccluderDepthPass::ClearDrawCommandListFunc()
{
  m_DrawCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphOccluderDepthPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphOccluderDepthPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_DrawCommandListFunc.IsValid())
  {
    m_DrawCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphOccluderDepthPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sOccluderInstancesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sOccluderDepthResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::DepthStencilWrite;
    output.m_RequiredState              = xiiGALResourceStateFlags::DepthWrite;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_OccluderDepthPass);
