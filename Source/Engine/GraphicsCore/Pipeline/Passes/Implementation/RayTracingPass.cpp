#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/RayTracingPass.h>

xiiRenderGraphRayTracingPass::xiiRenderGraphRayTracingPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("RayTracingPass");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Compute;
  m_PassDescription.m_bHasSideEffects = false;
}

void xiiRenderGraphRayTracingPass::SetPassName(xiiStringView sPassName)
{
  xiiStringBuilder sPassNameBuilder;
  sPassNameBuilder.Set(sPassName);
  m_PassDescription.m_sPassName.Assign(sPassNameBuilder.GetData());
}

void xiiRenderGraphRayTracingPass::SetQueueFlags(xiiBitflags<xiiGALCommandQueueFlags> queueFlags)
{
  m_PassDescription.m_QueueFlags = queueFlags;
}

void xiiRenderGraphRayTracingPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphRayTracingPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphRayTracingPass::AddInputResource(xiiHashedString sResourceName, xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags /*= xiiRenderGraphResourceAccessFlags::Read*/, xiiBitflags<xiiGALResourceStateFlags> requiredState /*= xiiGALResourceStateFlags::Unknown*/)
{
  xiiRenderGraphResourceUsage& input = m_PassDescription.m_Inputs.ExpandAndGetRef();
  input.m_sResourceName              = sResourceName;
  input.m_AccessFlags                = accessFlags;
  input.m_RequiredState              = requiredState;
}

void xiiRenderGraphRayTracingPass::AddOutputResource(xiiHashedString sResourceName, xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags /*= xiiRenderGraphResourceAccessFlags::Write*/, xiiBitflags<xiiGALResourceStateFlags> requiredState /*= xiiGALResourceStateFlags::Unknown*/)
{
  xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
  output.m_sResourceName              = sResourceName;
  output.m_AccessFlags                = accessFlags;
  output.m_RequiredState              = requiredState;
}

void xiiRenderGraphRayTracingPass::AddRayTracingSceneInput(xiiHashedString sResourceName)
{
  AddInputResource(sResourceName, xiiRenderGraphResourceAccessFlags::Read | xiiRenderGraphResourceAccessFlags::RayTracing, xiiGALResourceStateFlags::RayTracing);
}

void xiiRenderGraphRayTracingPass::AddBuildInputResource(xiiHashedString sResourceName)
{
  AddInputResource(sResourceName, xiiRenderGraphResourceAccessFlags::Read | xiiRenderGraphResourceAccessFlags::BuildASRead, xiiGALResourceStateFlags::BuildASRead);
}

void xiiRenderGraphRayTracingPass::AddBuildOutputResource(xiiHashedString sResourceName)
{
  AddOutputResource(sResourceName, xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::BuildASWrite, xiiGALResourceStateFlags::BuildASWrite);
}

void xiiRenderGraphRayTracingPass::AddUnorderedAccessOutput(xiiHashedString sResourceName)
{
  AddOutputResource(sResourceName, xiiRenderGraphResourceAccessFlags::Write | xiiRenderGraphResourceAccessFlags::UnorderedAccess, xiiGALResourceStateFlags::UnorderedAccess);
}

void xiiRenderGraphRayTracingPass::ClearInputResources()
{
  m_PassDescription.m_Inputs.Clear();
}

void xiiRenderGraphRayTracingPass::ClearOutputResources()
{
  m_PassDescription.m_Outputs.Clear();
}

void xiiRenderGraphRayTracingPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphRayTracingPass::SetDispatchRayTracingFunc(DispatchRayTracingFunc dispatchRayTracingFunc)
{
  m_DispatchRayTracingFunc = dispatchRayTracingFunc;
}

void xiiRenderGraphRayTracingPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_PostDispatchCommandListFunc = postDispatchCommandListFunc;
}

void xiiRenderGraphRayTracingPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphRayTracingPass::ClearDispatchRayTracingFunc()
{
  m_DispatchRayTracingFunc = {};
}

void xiiRenderGraphRayTracingPass::ClearPostDispatchCommandListFunc()
{
  m_PostDispatchCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphRayTracingPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphRayTracingPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
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

  if (!m_DispatchRayTracingFunc.IsValid())
  {
    return;
  }

  m_DispatchRayTracingFunc(*executionContext.m_pCommandList, executionContext);

  if (m_PostDispatchCommandListFunc.IsValid())
  {
    m_PostDispatchCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_RayTracingPass);
