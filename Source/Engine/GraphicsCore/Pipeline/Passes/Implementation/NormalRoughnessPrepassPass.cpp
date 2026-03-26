#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/NormalRoughnessPrepassPass.h>

xiiRenderGraphNormalRoughnessPrepassPass::xiiRenderGraphNormalRoughnessPrepassPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("NormalRoughnessPrepass");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sIndirectCommandBufferResourceName = xiiMakeHashedString("GpuIndirectDrawCommands");
  m_sIndirectCountBufferResourceName   = xiiMakeHashedString("GpuIndirectDrawCounts");
  m_sDepthResourceName                 = xiiMakeHashedString("SceneDepth");
  m_sNormalRoughnessResourceName       = xiiMakeHashedString("SceneNormalRoughness");

  RebuildResourceLayout();
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetIndirectCommandBufferResourceName(xiiHashedString sResourceName)
{
  m_sIndirectCommandBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetIndirectCountBufferResourceName(xiiHashedString sResourceName)
{
  m_sIndirectCountBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetDepthResourceName(xiiHashedString sResourceName)
{
  m_sDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetNormalRoughnessResourceName(xiiHashedString sResourceName)
{
  m_sNormalRoughnessResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphNormalRoughnessPrepassPass::SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc)
{
  m_DrawCommandListFunc = drawCommandListFunc;
}

void xiiRenderGraphNormalRoughnessPrepassPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphNormalRoughnessPrepassPass::ClearDrawCommandListFunc()
{
  m_DrawCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphNormalRoughnessPrepassPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphNormalRoughnessPrepassPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphNormalRoughnessPrepassPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sIndirectCommandBufferResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sIndirectCountBufferResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sDepthResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read | xiiRenderGraphResourceAccessFlags::DepthStencilReadOnly;
    input.m_RequiredState              = xiiGALResourceStateFlags::DepthRead;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sNormalRoughnessResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_NormalRoughnessPrepassPass);
