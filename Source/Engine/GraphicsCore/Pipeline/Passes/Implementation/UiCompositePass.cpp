#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/UiCompositePass.h>

xiiRenderGraphUiCompositePass::xiiRenderGraphUiCompositePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("UiComposite");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sSceneColorResourceName     = xiiMakeHashedString("LensFlareOutput");
  m_sUiRenderTargetResourceName = xiiMakeHashedString("UiCompositeOutput");

  RebuildResourceLayout();
}

void xiiRenderGraphUiCompositePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphUiCompositePass::SetSceneColorResourceName(xiiHashedString sResourceName)
{
  m_sSceneColorResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphUiCompositePass::SetUiRenderTargetResourceName(xiiHashedString sResourceName)
{
  m_sUiRenderTargetResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphUiCompositePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphUiCompositePass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphUiCompositePass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphUiCompositePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphUiCompositePass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphUiCompositePass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphUiCompositePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphUiCompositePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphUiCompositePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sSceneColorResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sUiRenderTargetResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::ReadWrite | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_UiCompositePass);
