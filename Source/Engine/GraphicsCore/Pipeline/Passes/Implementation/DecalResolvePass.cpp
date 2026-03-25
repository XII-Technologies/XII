#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DecalResolvePass.h>

xiiRenderGraphDecalResolvePass::xiiRenderGraphDecalResolvePass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("DecalClassification");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sDecalVolumesResourceName   = xiiMakeHashedString("DecalVolumes");
  m_sSceneDepthResourceName     = xiiMakeHashedString("SceneDepth");
  m_sDecalTileListsResourceName = xiiMakeHashedString("DecalTileLists");

  RebuildResourceLayout();
}

void xiiRenderGraphDecalResolvePass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphDecalResolvePass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphDecalResolvePass::SetDecalVolumesResourceName(xiiHashedString sResourceName)
{
  m_sDecalVolumesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDecalResolvePass::SetSceneDepthResourceName(xiiHashedString sResourceName)
{
  m_sSceneDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDecalResolvePass::SetDecalTileListsResourceName(xiiHashedString sResourceName)
{
  m_sDecalTileListsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphDecalResolvePass::SetGBufferInputResourceName(xiiHashedString sResourceName)
{
  SetDecalVolumesResourceName(sResourceName);
}

void xiiRenderGraphDecalResolvePass::SetDecalOutputResourceName(xiiHashedString sResourceName)
{
  SetDecalTileListsResourceName(sResourceName);
}

void xiiRenderGraphDecalResolvePass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphDecalResolvePass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphDecalResolvePass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphDecalResolvePass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphDecalResolvePass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphDecalResolvePass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(m_uiDispatchThreadGroupsX, m_uiDispatchThreadGroupsY, m_uiDispatchThreadGroupsZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphDecalResolvePass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sDecalVolumesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sSceneDepthResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sDecalTileListsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_DecalResolvePass);
