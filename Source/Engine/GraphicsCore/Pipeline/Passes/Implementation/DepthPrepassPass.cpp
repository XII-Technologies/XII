#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DepthPrepassPass.h>

xiiRenderGraphDepthPrepassPass::xiiRenderGraphDepthPrepassPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("DepthPrepass");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sVisibleInstancesResourceName = xiiMakeHashedString("GpuVisibleInstances");
  m_sDepthBufferResourceName      = xiiMakeHashedString("SceneDepth");

  RebuildResourceLayout();
}

void xiiRenderGraphDepthPrepassPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphDepthPrepassPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphDepthPrepassPass::SetVisibleInstancesResourceName(xiiHashedString sResourceName)
{
  m_sVisibleInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDepthPrepassPass::SetDepthBufferResourceName(xiiHashedString sResourceName)
{
  m_sDepthBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDepthPrepassPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphDepthPrepassPass::SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc)
{
  m_DrawCommandListFunc = drawCommandListFunc;
}

void xiiRenderGraphDepthPrepassPass::SetPostCommandListFunc(PostCommandListFunc postCommandListFunc)
{
  m_PostCommandListFunc = postCommandListFunc;
}

void xiiRenderGraphDepthPrepassPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphDepthPrepassPass::ClearDrawCommandListFunc()
{
  m_DrawCommandListFunc = {};
}

void xiiRenderGraphDepthPrepassPass::ClearPostCommandListFunc()
{
  m_PostCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphDepthPrepassPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphDepthPrepassPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_PostCommandListFunc.IsValid())
  {
    m_PostCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphDepthPrepassPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sVisibleInstancesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sDepthBufferResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::DepthStencilWrite;
    output.m_RequiredState              = xiiGALResourceStateFlags::DepthWrite;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_DepthPrepassPass);
