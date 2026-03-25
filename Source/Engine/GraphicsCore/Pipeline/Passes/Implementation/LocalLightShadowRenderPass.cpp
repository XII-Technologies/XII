#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/LocalLightShadowRenderPass.h>

xiiRenderGraphLocalLightShadowRenderPass::xiiRenderGraphLocalLightShadowRenderPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("LocalLightShadowRender");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sLocalShadowCastersResourceName      = xiiMakeHashedString("LocalShadowCasters");
  m_sLocalShadowMaterialBinsResourceName = xiiMakeHashedString("LocalShadowMaterialBins");
  m_sLocalShadowModeBinsResourceName     = xiiMakeHashedString("LocalShadowModeBins");
  m_sLocalShadowAtlasPagesResourceName   = xiiMakeHashedString("LocalShadowAtlasPages");

  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowCastersResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowCastersResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowMaterialBinsResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowMaterialBinsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowModeBinsResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowModeBinsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowAtlasPagesResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowAtlasPagesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphLocalLightShadowRenderPass::SetExecuteCommandListFunc(ExecuteCommandListFunc executeCommandListFunc)
{
  m_ExecuteCommandListFunc = executeCommandListFunc;
}

void xiiRenderGraphLocalLightShadowRenderPass::SetPostExecuteCommandListFunc(PostExecuteCommandListFunc postExecuteCommandListFunc)
{
  m_PostExecuteCommandListFunc = postExecuteCommandListFunc;
}

void xiiRenderGraphLocalLightShadowRenderPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphLocalLightShadowRenderPass::ClearExecuteCommandListFunc()
{
  m_ExecuteCommandListFunc = {};
}

void xiiRenderGraphLocalLightShadowRenderPass::ClearPostExecuteCommandListFunc()
{
  m_PostExecuteCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphLocalLightShadowRenderPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphLocalLightShadowRenderPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphLocalLightShadowRenderPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLocalShadowCastersResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::IndirectArgument | xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLocalShadowMaterialBinsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLocalShadowModeBinsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLocalShadowAtlasPagesResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::DepthWrite;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_LocalLightShadowRenderPass);
