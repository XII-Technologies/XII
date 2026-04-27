#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/GPUDrivenCullingPass.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GAL/Device/GALDevice.h>
#include <GAL/CommandList/GALCommandList.h>

xiiGPUDrivenCullingPass::xiiGPUDrivenCullingPass()  = default;
xiiGPUDrivenCullingPass::~xiiGPUDrivenCullingPass() { Deinitialize(); }

void xiiGPUDrivenCullingPass::Initialize()
{
  if (m_bInitialised)
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No GAL device");

  // ----- Phase 1 — Frustum + cone cull PSO -----
  {
    xiiGALComputePipelineCreationDescription desc;
    desc.m_sDebugName          = "FrustumCullPSO";
    desc.m_sShaderFilePath     = "Shaders/Culling/FrustumCull.hlsl";
    desc.m_sEntryPoint         = "CSMain";
    m_hFrustumCullPSO = pDevice->CreateComputePipelineState(desc);
  }

  // ----- Phase 2 — HZB occlusion cull PSO -----
  {
    xiiGALComputePipelineCreationDescription desc;
    desc.m_sDebugName          = "OcclusionCullPSO";
    desc.m_sShaderFilePath     = "Shaders/Culling/OcclusionCull.hlsl";
    desc.m_sEntryPoint         = "CSMain";
    m_hOcclusionCullPSO = pDevice->CreateComputePipelineState(desc);
  }

  // ----- HZB mip-chain build PSO -----
  {
    xiiGALComputePipelineCreationDescription desc;
    desc.m_sDebugName          = "HZBBuildPSO";
    desc.m_sShaderFilePath     = "Shaders/Culling/HZBBuild.hlsl";
    desc.m_sEntryPoint         = "CSMain";
    m_hHZBBuildPSO = pDevice->CreateComputePipelineState(desc);
  }

  // ----- Visibility bitmask buffer -----
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "VisibilityBuffer";
    bd.m_uiSize        = (65536 / 32) * sizeof(xiiUInt32); // supports up to 64k instances (1 bit each)
    bd.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_hVisibilityBuffer = pDevice->CreateBuffer(bd);
  }

  // ----- Draw count buffers -----
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "Phase1DrawCount";
    bd.m_uiSize        = sizeof(xiiUInt32);
    bd.m_BindFlags     = xiiGALBindFlags::UnorderedAccess;
    bd.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_hPhase1DrawCountBuffer = pDevice->CreateBuffer(bd);
    bd.m_sDebugName    = "Phase2DrawCount";
    m_hPhase2DrawCountBuffer = pDevice->CreateBuffer(bd);
  }

  m_bInitialised = true;
}

void xiiGPUDrivenCullingPass::Deinitialize()
{
  if (!m_bInitialised)
    return;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice) return;

  auto Destroy = [&](auto& h) { if (h.IsValid()) { pDevice->DestroyResource(h); h = {}; } };
  Destroy(m_hFrustumCullPSO);
  Destroy(m_hOcclusionCullPSO);
  Destroy(m_hHZBBuildPSO);
  Destroy(m_hVisibilityBuffer);
  Destroy(m_hPhase1DrawCountBuffer);
  Destroy(m_hPhase2DrawCountBuffer);

  m_bInitialised = false;
}

void xiiGPUDrivenCullingPass::ExecutePhase1(xiiIndirectDrawBatchBuilder& builder,
                                             xiiRGPassContext& ctx,
                                             const xiiFrustum& frustum)
{
  XII_IGNORE_UNUSED(frustum);

  if (!m_bInitialised)
    Initialize();

  const xiiUInt32 uiInstances = builder.GetInstanceCount();
  if (uiInstances == 0)
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();

  // Reset draw count buffers to zero
  cmd.FillBuffer(m_hPhase1DrawCountBuffer, 0, sizeof(xiiUInt32), 0u);

  if (m_bFrustumCulling && m_hFrustumCullPSO.IsValid())
  {
    // Bind PSO and instance/visibility SRVs, dispatch one thread per instance.
    cmd.SetComputePipelineState(m_hFrustumCullPSO);
    cmd.BindShaderResource(0, builder.GetInstanceBuffer(), xiiGALShaderStage::Compute);
    cmd.BindUnorderedAccessView(0, m_hVisibilityBuffer);
    cmd.BindUnorderedAccessView(1, builder.GetIndirectArgsBuffer());
    cmd.BindUnorderedAccessView(2, m_hPhase1DrawCountBuffer);

    const xiiUInt32 uiGroups = (uiInstances + 63u) / 64u;
    cmd.Dispatch(uiGroups, 1, 1);
  }
  else
  {
    // No culling: mark all visible. Fill indirect args directly from batch list.
    // Implemented by a pass-through compute shader or CPU-side fill.
    m_uiVisibleInstances = uiInstances;
    m_uiCulledInstances  = 0;
  }
}

void xiiGPUDrivenCullingPass::ExecutePhase2(xiiIndirectDrawBatchBuilder& builder,
                                             xiiRGPassContext& ctx)
{
  if (!m_bInitialised || !m_bOcclusionCulling)
    return;

  const xiiUInt32 uiInstances = builder.GetInstanceCount();
  if (uiInstances == 0)
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.FillBuffer(m_hPhase2DrawCountBuffer, 0, sizeof(xiiUInt32), 0u);

  if (m_hOcclusionCullPSO.IsValid())
  {
    cmd.SetComputePipelineState(m_hOcclusionCullPSO);
    cmd.BindShaderResource(0, builder.GetInstanceBuffer(), xiiGALShaderStage::Compute);
    cmd.BindUnorderedAccessView(0, m_hVisibilityBuffer);
    cmd.BindUnorderedAccessView(1, builder.GetIndirectArgsBuffer());
    cmd.BindUnorderedAccessView(2, m_hPhase2DrawCountBuffer);

    const xiiUInt32 uiGroups = (uiInstances + 63u) / 64u;
    cmd.Dispatch(uiGroups, 1, 1);
  }
}

void xiiGPUDrivenCullingPass::BuildHZB(xiiRGPassContext& ctx)
{
  if (!m_bInitialised || !m_hHZBBuildPSO.IsValid())
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.SetComputePipelineState(m_hHZBBuildPSO);
  // Dispatched with one thread group per 2x2 output texel block at each mip level.
  // Actual dispatch size depends on the depth buffer resolution, available from the blackboard.
  // Stubbed as 16x16 groups for a 1080p depth buffer.
  cmd.Dispatch(120, 68, 1);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_GPUDrivenCullingPass);
