#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/MotionVectorsPass.h>

xiiRenderGraphMotionVectorsPass::xiiRenderGraphMotionVectorsPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("MotionVectors");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sDepthResourceName         = xiiMakeHashedString("SceneDepth");
  m_sMotionVectorsResourceName = xiiMakeHashedString("MotionVectors");

  RebuildResourceLayout();
}

void xiiRenderGraphMotionVectorsPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphMotionVectorsPass::SetDepthResourceName(xiiHashedString sResourceName)
{
  m_sDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphMotionVectorsPass::SetMotionVectorsResourceName(xiiHashedString sResourceName)
{
  m_sMotionVectorsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphMotionVectorsPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphMotionVectorsPass::SetDrawCommandListFunc(DrawCommandListFunc drawCommandListFunc)
{
  m_DrawCommandListFunc = drawCommandListFunc;
}

void xiiRenderGraphMotionVectorsPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphMotionVectorsPass::ClearDrawCommandListFunc()
{
  m_DrawCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphMotionVectorsPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphMotionVectorsPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphMotionVectorsPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sDepthResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read | xiiRenderGraphResourceAccessFlags::DepthStencilReadOnly;
    input.m_RequiredState              = xiiGALResourceStateFlags::DepthRead;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sMotionVectorsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_MotionVectorsPass);
