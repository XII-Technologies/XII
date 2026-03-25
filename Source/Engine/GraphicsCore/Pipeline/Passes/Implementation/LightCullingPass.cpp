#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/LightCullingPass.h>

xiiRenderGraphLightCullingPass::xiiRenderGraphLightCullingPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("LightCulling");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sDepthBufferResourceName = xiiMakeHashedString("SceneDepth");
  m_sLightDataResourceName   = xiiMakeHashedString("VisibleLights");
  m_sLightGridResourceName   = xiiMakeHashedString("LightGrid");

  RebuildResourceLayout();
}

void xiiRenderGraphLightCullingPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphLightCullingPass::SetThreadGroupSize(xiiUInt32 uiThreadGroupSizeX, xiiUInt32 uiThreadGroupSizeY /*= 1U*/, xiiUInt32 uiThreadGroupSizeZ /*= 1U*/)
{
  m_uiThreadGroupSizeX = xiiMath::Max(1U, uiThreadGroupSizeX);
  m_uiThreadGroupSizeY = xiiMath::Max(1U, uiThreadGroupSizeY);
  m_uiThreadGroupSizeZ = xiiMath::Max(1U, uiThreadGroupSizeZ);
}

void xiiRenderGraphLightCullingPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = uiThreadGroupCountX;
  m_uiDispatchThreadGroupsY = uiThreadGroupCountY;
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphLightCullingPass::SetDepthBufferResourceName(xiiHashedString sResourceName)
{
  m_sDepthBufferResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightCullingPass::SetLightDataResourceName(xiiHashedString sResourceName)
{
  m_sLightDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightCullingPass::SetLightGridResourceName(xiiHashedString sResourceName)
{
  m_sLightGridResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLightCullingPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphLightCullingPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphLightCullingPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphLightCullingPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphLightCullingPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphLightCullingPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_uiDispatchThreadGroupsX == 0U || m_uiDispatchThreadGroupsY == 0U)
  {
    return;
  }

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(m_uiDispatchThreadGroupsX, m_uiDispatchThreadGroupsY, m_uiDispatchThreadGroupsZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphLightCullingPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sDepthBufferResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sLightDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLightGridResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_LightCullingPass);
