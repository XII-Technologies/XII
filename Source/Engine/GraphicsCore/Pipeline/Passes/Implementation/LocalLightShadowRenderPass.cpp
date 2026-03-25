#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/LocalLightShadowRenderPass.h>

xiiRenderGraphLocalLightShadowRenderPass::xiiRenderGraphLocalLightShadowRenderPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("LocalLightShadowRender");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = false;

  m_sLocalShadowDataResourceName       = xiiMakeHashedString("LocalShadowData");
  m_sLocalShadowDepthAtlasResourceName = xiiMakeHashedString("LocalShadowDepthAtlas");

  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowDataResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLocalLightShadowRenderPass::SetLocalShadowDepthAtlasResourceName(xiiHashedString sResourceName)
{
  m_sLocalShadowDepthAtlasResourceName = sResourceName;
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
    input.m_sResourceName              = m_sLocalShadowDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::IndirectArgument | xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLocalShadowDepthAtlasResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::RenderTarget;
    output.m_RequiredState              = xiiGALResourceStateFlags::DepthWrite;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_LocalLightShadowRenderPass);
