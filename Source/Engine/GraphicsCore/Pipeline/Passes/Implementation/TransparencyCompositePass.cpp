#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/TransparencyCompositePass.h>

xiiRenderGraphTransparencyCompositePass::xiiRenderGraphTransparencyCompositePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("TransparencyComposite");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sTransparencyDepthResourceName = xiiMakeHashedString("TransparencyDepth");
  m_sSceneColorResourceName        = xiiMakeHashedString("SceneColor");

  RebuildResourceLayout();
}

void xiiRenderGraphTransparencyCompositePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphTransparencyCompositePass::SetTransparencyDepthResourceName(xiiHashedString sResourceName)
{
  m_sTransparencyDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphTransparencyCompositePass::SetSceneColorResourceName(xiiHashedString sResourceName)
{
  m_sSceneColorResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphTransparencyCompositePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphTransparencyCompositePass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphTransparencyCompositePass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphTransparencyCompositePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphTransparencyCompositePass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphTransparencyCompositePass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphTransparencyCompositePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphTransparencyCompositePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_ExecuteCommandListFunc.IsValid())
  {
    m_ExecuteCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (m_PostExecuteCommandListFunc.IsValid())
  {
    m_PostExecuteCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphTransparencyCompositePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sTransparencyDepthResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sSceneColorResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::ReadWrite | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_TransparencyCompositePass);
