#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/BloomBlurPass.h>

xiiRenderGraphBloomBlurPass::xiiRenderGraphBloomBlurPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("BloomBlur");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;

  m_sBloomExtractInputResourceName = xiiMakeHashedString("BloomExtractOutput");
  m_sBloomBlurOutputResourceName   = xiiMakeHashedString("BloomBlurOutput");

  RebuildResourceLayout();
}

void xiiRenderGraphBloomBlurPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphBloomBlurPass::SetDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/)
{
  m_uiDispatchThreadGroupsX = xiiMath::Max(1U, uiThreadGroupCountX);
  m_uiDispatchThreadGroupsY = xiiMath::Max(1U, uiThreadGroupCountY);
  m_uiDispatchThreadGroupsZ = xiiMath::Max(1U, uiThreadGroupCountZ);
}

void xiiRenderGraphBloomBlurPass::SetBloomExtractInputResourceName(xiiHashedString sResourceName)
{
  m_sBloomExtractInputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphBloomBlurPass::SetBloomBlurOutputResourceName(xiiHashedString sResourceName)
{
  m_sBloomBlurOutputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphBloomBlurPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphBloomBlurPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphBloomBlurPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphBloomBlurPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphBloomBlurPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphBloomBlurPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderGraphBloomBlurPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
    input.m_sResourceName              = m_sBloomExtractInputResourceName;
    input.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Read;
    input.m_RequiredState              = xiiGALResourceStateFlags::ShaderResource;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sBloomBlurOutputResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess;
    output.m_RequiredState              = xiiGALResourceStateFlags::UnorderedAccess;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_BloomBlurPass);
