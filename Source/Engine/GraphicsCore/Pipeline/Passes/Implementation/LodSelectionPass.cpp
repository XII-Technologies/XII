#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/LodSelectionPass.h>

xiiRenderGraphLodSelectionPass::xiiRenderGraphLodSelectionPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("LodSelection");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sGpuSceneBoundsResourceName    = xiiMakeHashedString("GpuSceneBounds");
  m_sGpuSceneInstancesResourceName = xiiMakeHashedString("GpuSceneInstances");
  m_sCameraDataResourceName        = xiiMakeHashedString("FrameConstants");
  m_sLodSelectionResourceName      = xiiMakeHashedString("GpuLodSelections");
  m_sDrawMetadataResourceName      = xiiMakeHashedString("GpuDrawMetadata");

  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphLodSelectionPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = uiThreadGroupCountX;
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphLodSelectionPass::SetGpuSceneBoundsResourceName(xiiHashedString sResourceName)
{
  m_sGpuSceneBoundsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetGpuSceneInstancesResourceName(xiiHashedString sResourceName)
{
  m_sGpuSceneInstancesResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetCameraDataResourceName(xiiHashedString sResourceName)
{
  m_sCameraDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetLodSelectionResourceName(xiiHashedString sResourceName)
{
  m_sLodSelectionResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetDrawMetadataResourceName(xiiHashedString sResourceName)
{
  m_sDrawMetadataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphLodSelectionPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphLodSelectionPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphLodSelectionPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphLodSelectionPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphLodSelectionPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphLodSelectionPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (m_uiDispatchThreadGroupsX == 0U)
  {
    return;
  }

  executionContext.m_pCommandList->DispatchCompute(xiiGALDispatchComputeDescription(m_uiDispatchThreadGroupsX, m_uiDispatchThreadGroupsY, m_uiDispatchThreadGroupsZ));

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphLodSelectionPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sGpuSceneBoundsResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sGpuSceneInstancesResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sCameraDataResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ConstantBuffer;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLodSelectionResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sDrawMetadataResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_LodSelectionPass);
