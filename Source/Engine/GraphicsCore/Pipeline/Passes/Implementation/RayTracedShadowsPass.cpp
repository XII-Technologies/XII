#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/RayTracedShadowsPass.h>

xiiRenderGraphRayTracedShadowsPass::xiiRenderGraphRayTracedShadowsPass()
{
  m_RayTracingPass.SetPassName("RayTracedShadows");
  m_RayTracingPass.SetQueueFlags(xiiGALCommandQueueFlags::Compute);
  m_RayTracingPass.SetHasSideEffects(false);

  m_sSceneTlasResourceName = xiiMakeHashedString("SceneTLAS");
  m_sDepthResourceName = xiiMakeHashedString("SceneDepth");
  m_sNormalResourceName = xiiMakeHashedString("SceneNormals");
  m_sLightDataResourceName = xiiMakeHashedString("ShadowLights");
  m_sShadowMaskResourceName = xiiMakeHashedString("RayTracedShadowMask");
  m_sHistoryInputResourceName = xiiMakeHashedString("RayTracedShadowHistoryIn");
  m_sHistoryOutputResourceName = xiiMakeHashedString("RayTracedShadowHistoryOut");

  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetEnabled(bool bEnabled)
{
  m_RayTracingPass.SetEnabled(bEnabled);
}

void xiiRenderGraphRayTracedShadowsPass::SetDenoiserHistoryEnabled(bool bEnabled)
{
  if (m_bDenoiserHistoryEnabled == bEnabled)
  {
    return;
  }

  m_bDenoiserHistoryEnabled = bEnabled;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetSceneTlasResourceName(xiiHashedString sResourceName)
{
  m_sSceneTlasResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetDepthResourceName(xiiHashedString sResourceName)
{
  m_sDepthResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetNormalResourceName(xiiHashedString sResourceName)
{
  m_sNormalResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetLightDataResourceName(xiiHashedString sResourceName)
{
  m_sLightDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetShadowMaskResourceName(xiiHashedString sResourceName)
{
  m_sShadowMaskResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetHistoryInputResourceName(xiiHashedString sResourceName)
{
  m_sHistoryInputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetHistoryOutputResourceName(xiiHashedString sResourceName)
{
  m_sHistoryOutputResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphRayTracedShadowsPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_RayTracingPass.SetSetupCommandListFunc(setupCommandListFunc);
}

void xiiRenderGraphRayTracedShadowsPass::SetDispatchRayTracingFunc(DispatchRayTracingFunc dispatchRayTracingFunc)
{
  m_RayTracingPass.SetDispatchRayTracingFunc(dispatchRayTracingFunc);
}

void xiiRenderGraphRayTracedShadowsPass::SetPostDispatchCommandListFunc(PostDispatchCommandListFunc postDispatchCommandListFunc)
{
  m_RayTracingPass.SetPostDispatchCommandListFunc(postDispatchCommandListFunc);
}

void xiiRenderGraphRayTracedShadowsPass::ClearSetupCommandListFunc()
{
  m_RayTracingPass.ClearSetupCommandListFunc();
}

void xiiRenderGraphRayTracedShadowsPass::ClearDispatchRayTracingFunc()
{
  m_RayTracingPass.ClearDispatchRayTracingFunc();
}

void xiiRenderGraphRayTracedShadowsPass::ClearPostDispatchCommandListFunc()
{
  m_RayTracingPass.ClearPostDispatchCommandListFunc();
}

const xiiRenderGraphPassDescription& xiiRenderGraphRayTracedShadowsPass::GetDescription() const
{
  return m_RayTracingPass.GetDescription();
}

void xiiRenderGraphRayTracedShadowsPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  m_RayTracingPass.RecordCommands(executionContext);
}

void xiiRenderGraphRayTracedShadowsPass::RebuildResourceLayout()
{
  m_RayTracingPass.ClearInputResources();
  m_RayTracingPass.ClearOutputResources();

  m_RayTracingPass.AddRayTracingSceneInput(m_sSceneTlasResourceName);
  m_RayTracingPass.AddInputResource(m_sDepthResourceName, xiiRenderGraphResourceAccessFlags::Read, xiiGALResourceStateFlags::ShaderResource);
  m_RayTracingPass.AddInputResource(m_sNormalResourceName, xiiRenderGraphResourceAccessFlags::Read, xiiGALResourceStateFlags::ShaderResource);
  m_RayTracingPass.AddInputResource(m_sLightDataResourceName, xiiRenderGraphResourceAccessFlags::Read, xiiGALResourceStateFlags::ShaderResource);
  m_RayTracingPass.AddUnorderedAccessOutput(m_sShadowMaskResourceName);

  if (m_bDenoiserHistoryEnabled)
  {
    m_RayTracingPass.AddInputResource(m_sHistoryInputResourceName, xiiRenderGraphResourceAccessFlags::Read, xiiGALResourceStateFlags::ShaderResource);
    m_RayTracingPass.AddUnorderedAccessOutput(m_sHistoryOutputResourceName);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_RayTracedShadowsPass);
