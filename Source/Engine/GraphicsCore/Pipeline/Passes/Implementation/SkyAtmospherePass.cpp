#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/SkyAtmospherePass.h>

xiiRenderGraphSkyAtmospherePass::xiiRenderGraphSkyAtmospherePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("SkyAtmosphere");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sCameraDataResourceName = xiiMakeHashedString("FrameConstants");
  m_sSceneColorResourceName = xiiMakeHashedString("SceneColor");

  RebuildResourceLayout();
}

void xiiRenderGraphSkyAtmospherePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphSkyAtmospherePass::SetCameraDataResourceName(xiiHashedString sResourceName)
{
  m_sCameraDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSkyAtmospherePass::SetSceneColorResourceName(xiiHashedString sResourceName)
{
  m_sSceneColorResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphSkyAtmospherePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphSkyAtmospherePass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphSkyAtmospherePass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphSkyAtmospherePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphSkyAtmospherePass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphSkyAtmospherePass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphSkyAtmospherePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphSkyAtmospherePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphSkyAtmospherePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sCameraDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sSceneColorResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::ReadWrite | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::RenderTarget;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_SkyAtmospherePass);
