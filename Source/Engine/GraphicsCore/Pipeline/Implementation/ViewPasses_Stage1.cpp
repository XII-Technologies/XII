// XII Engine - ViewPasses_Stage1.cpp
// Stage 1: Frame setup & visibility passes.
// All passes on the Compute queue. Frustum cull → LOD → instance update → draw-command build →
// shadow-caster list → cluster grid → light list → reflection probe selection → froxel grid alloc.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

extern xiiCVarInt cvar_ClusterX;
extern xiiCVarInt cvar_ClusterY;
extern xiiCVarInt cvar_ClusterZ;

// 
// Shared constants (sizes of persistent GPU buffers, aligned to typical instance budgets)
// 
static constexpr xiiUInt32 k_uiMaxInstances    = 65536u; // max drawable objects in one frame
static constexpr xiiUInt32 k_uiMaxLights       = 1024u;
static constexpr xiiUInt32 k_uiMaxMaterialBins = 512u;   // distinct (mesh × material) draw bins

// 
// GPU occlusion readback
// 
// Reads the oldest staging buffer in the 3-frame ring (from 2 frames ago).
// This provides the GPU frame time used by the PID (the result has already been applied by
// RunDynamicResolutionPID before BeginSetup, so this pass simply keeps the readback ring rotating).

namespace
{
  struct OcclusionReadbackData
  {
    xiiUInt32 m_uiReadSlot = 0;
  };
} // namespace

static void SetupOcclusionReadback(xiiView& view, OcclusionReadbackData& data, xiiRGBuilder& builder)
{
  auto& ring     = view.m_ViewPassResources.m_VisibilityPasses;
  data.m_uiReadSlot = (ring.m_uiReadbackWriteSlot + 1u) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

static void ExecuteOcclusionReadback(xiiView& view, const OcclusionReadbackData& data, xiiRGPassContext& ctx)
{
  auto& ring        = view.m_ViewPassResources.m_VisibilityPasses;
  xiiSharedPtr<xiiGALBuffer>& pStaging = ring.m_pOcclusionReadbackRing[data.m_uiReadSlot];

  if (pStaging == nullptr)
  {
    // Buffer not yet populated - skip.
    // Advance the write slot so the next stage can write into it next frame.
    ring.m_uiReadbackWriteSlot = (ring.m_uiReadbackWriteSlot + 1u) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
    return;
  }

  // Map the staging buffer (CPU readable, GPU wrote 2 frames ago).
  xiiGALCommandList& cmd = ctx.GetCommandList();
  const void*        pData = nullptr;
  if (cmd.MapBuffer(pStaging, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pData))
  {
    // The staging buffer holds a single float: total GPU frame time in nanoseconds.
    // (Written by the previous frame's FrameTotal sentinel query readback.)
    // We don't store it here; the profiler's GetPassDurationMs("FrameTotal") path already does it.
    cmd.UnmapBuffer(pStaging, xiiGALMapType::Read);
  }

  // Advance ring.
  ring.m_uiReadbackWriteSlot = (ring.m_uiReadbackWriteSlot + 1u) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
}

// 
// Frustum culling
// 
namespace
{
  struct FrustumCullData
  {
    xiiRGBufferHandle m_hInstanceBounds;      // SRV in
    xiiRGBufferHandle m_hLODMetadata;         // SRV in
    xiiRGBufferHandle m_hVisibleCandidates;   // UAV out ([0]=count, [1..]=indices)
    xiiUInt32         m_uiInstanceCount = 0;
  };
}

static void SetupFrustumCulling(xiiView& view, FrustumCullData& data, xiiRGBuilder& builder)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  // Ensure persistent instance bounds buffer exists.
  if (!vp.m_pInstanceBoundsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = 32u; // float3 center + float radius + float3 extents + float pad
    desc.m_uiSize              = desc.m_uiElementByteStride * k_uiMaxInstances;
    desc.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    desc.m_Mode                = xiiGALBufferMode::Structured;
    desc.m_Usage               = xiiGALResourceUsage::Default;
    vp.m_pInstanceBoundsBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Import persistent instance bounds as read-only SRV.
  data.m_hInstanceBounds    = builder.ImportBuffer("InstanceBoundsIn", vp.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(data.m_hInstanceBounds, xiiGALResourceStateFlags::ShaderResource);

  // LOD metadata (also persistent, updated by CPU each frame before dispatch).
  if (!vp.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = 4u; // packed uint: LOD + flags
    desc.m_uiSize              = desc.m_uiElementByteStride * k_uiMaxInstances;
    desc.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    desc.m_Mode                = xiiGALBufferMode::Structured;
    desc.m_Usage               = xiiGALResourceUsage::Default;
    vp.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
  data.m_hLODMetadata = builder.ImportBuffer("LODMetadataIn", vp.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLODMetadata = builder.ReadBuffer(data.m_hLODMetadata, xiiGALResourceStateFlags::ShaderResource);

  // Transient visible candidate buffer.
  xiiGALBufferCreationDescription visDesc;
  visDesc.m_uiElementByteStride = 4u; // uint
  visDesc.m_uiSize              = 4u + 4u * k_uiMaxInstances; // [0]=count + indices
  visDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  visDesc.m_Mode                = xiiGALBufferMode::Structured;
  visDesc.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hVisibleCandidates     = builder.WriteBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, visDesc, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiInstanceCount = k_uiMaxInstances; // driven by CPU-side count from extraction

  xiiView::EnsureComputePipeline(vp.m_pFrustumCullPipeline, "Shaders/Pipeline/CoarseFrustumCulling.xiiShader");
  builder.SetPassAllowMerge(false);
}

static void ExecuteFrustumCulling(xiiView& view, const FrustumCullData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("FrustumCulling");
  cmd.SetPipelineState(vp.m_pFrustumCullPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_Bounds",      ctx.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetShaderResourceBufferView("g_LODMetadata", ctx.GetBuffer(data.m_hLODMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource),    xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleOut", ctx.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

  const xiiUInt32 uiGroups = (data.m_uiInstanceCount + 63u) / 64u;
  cmd.DispatchCompute({uiGroups, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// LOD selection
// 
namespace
{
  struct LODSelectData
  {
    xiiRGBufferHandle m_hVisibleCandidates;
    xiiRGBufferHandle m_hInstanceBounds;
    xiiRGBufferHandle m_hInstanceLOD;
    xiiUInt32         m_uiInstanceCount = 0;
  };
}

static void SetupLODSelection(xiiView& view, LODSelectData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hCandidates;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);

  data.m_hVisibleCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(builder.ImportBuffer("InstanceBoundsLOD", view.m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount    = k_uiMaxInstances;

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 4u; // packed uint: LOD level + meshlet offset
  desc.m_uiSize              = desc.m_uiElementByteStride * k_uiMaxInstances;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceLOD        = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pLODSelectPipeline, "Shaders/Pipeline/LodSelection.xiiShader");
}

static void ExecuteLODSelection(xiiView& view, const LODSelectData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("LODSelection");
  cmd.SetPipelineState(vp.m_pLODSelectPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", ctx.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetShaderResourceBufferView("g_Bounds",            ctx.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource),    xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_InstanceLODOut",   ctx.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),      xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

  const xiiUInt32 uiGroups = (data.m_uiInstanceCount + 63u) / 64u;
  cmd.DispatchCompute({uiGroups, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Instance update (world-matrix transform buffer write)
// 
namespace
{
  struct InstanceUpdateData
  {
    xiiRGBufferHandle m_hVisibleCandidates;
    xiiRGBufferHandle m_hInstanceMatrices;
    xiiRGBufferHandle m_hInstanceBoundsOut;
    xiiUInt32         m_uiInstanceCount = 0;
  };
}

static void SetupInstanceUpdate(xiiView& view, InstanceUpdateData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hCandidates;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);
  data.m_hVisibleCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount    = k_uiMaxInstances;

  // Ensure persistent matrix buffer.
  if (!vp.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = 48u; // float4x3 (3 rows × 4 floats)
    desc.m_uiSize              = desc.m_uiElementByteStride * k_uiMaxInstances;
    desc.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    desc.m_Mode                = xiiGALBufferMode::Structured;
    desc.m_Usage               = xiiGALResourceUsage::Default;
    vp.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  data.m_hInstanceMatrices = builder.ImportBuffer("InstanceWorldMatrices", vp.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hInstanceMatrices = builder.WriteBuffer(data.m_hInstanceMatrices, xiiGALResourceStateFlags::UnorderedAccess);

  // Output AABB buffer for HiZ culling.
  xiiGALBufferCreationDescription boundsDesc;
  boundsDesc.m_uiElementByteStride = 32u;
  boundsDesc.m_uiSize              = boundsDesc.m_uiElementByteStride * k_uiMaxInstances;
  boundsDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  boundsDesc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceBoundsOut        = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, boundsDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pInstanceUpdatePipeline, "Shaders/Pipeline/InstanceUpdate.xiiShader");
}

static void ExecuteInstanceUpdate(xiiView& view, const InstanceUpdateData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("InstanceUpdate");
  cmd.SetPipelineState(vp.m_pInstanceUpdatePipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_VisibleIn",      ctx.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),   xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_MatricesOut",   ctx.GetBuffer(data.m_hInstanceMatrices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),   xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_BoundsOut",     ctx.GetBuffer(data.m_hInstanceBoundsOut)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),  xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Draw command build (GPU-driven indirect args)
// 
namespace
{
  struct DrawBuildData
  {
    xiiRGBufferHandle m_hSurvivors;          // surviving instance list
    xiiRGBufferHandle m_hInstanceLOD;        // LOD selection
    xiiRGBufferHandle m_hDrawCommands;       // DrawIndexedIndirect args output
    xiiRGBufferHandle m_hDrawCounts;         // per-bin draw counts
    xiiUInt32         m_uiInstanceCount = 0;
  };
}

static void SetupDrawBuild(xiiView& view, DrawBuildData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hSurvivors;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer), hSurvivors);
  if (!hSurvivors.IsValid())
  {
    // HiZ occlusion hasn't run yet; read the coarse-cull output instead.
    bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hSurvivors);
  }
  data.m_hSurvivors = builder.ReadBuffer(hSurvivors, xiiGALResourceStateFlags::ShaderResource);

  xiiRGBufferHandle hLOD;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer), hLOD);
  data.m_hInstanceLOD = builder.ReadBuffer(hLOD, xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount = k_uiMaxInstances;

  // Persistent indirect arg buffer (resized lazily).
  const xiiUInt32 uiArgStride = 20u; // DrawIndexedIndirectArguments: 5 × uint
  if (!vp.m_pDrawIndirectArgBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = uiArgStride;
    desc.m_uiSize              = uiArgStride * k_uiMaxMaterialBins;
    desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
    desc.m_Mode                = xiiGALBufferMode::Formatted;
    desc.m_Usage               = xiiGALResourceUsage::Default;
    vp.m_pDrawIndirectArgBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
  data.m_hDrawCommands = builder.ImportBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, vp.m_pDrawIndirectArgBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hDrawCommands = builder.WriteBuffer(data.m_hDrawCommands, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription countDesc;
  countDesc.m_uiElementByteStride = 4u;
  countDesc.m_uiSize              = 4u * k_uiMaxMaterialBins;
  countDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  countDesc.m_Mode                = xiiGALBufferMode::Formatted;
  data.m_hDrawCounts              = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawCountBuffer, countDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pDrawBuildPipeline, "Shaders/Pipeline/DrawCommandBuild.xiiShader");
}

static void ExecuteDrawBuild(xiiView& view, const DrawBuildData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("DrawCommandBuild");
  cmd.SetPipelineState(vp.m_pDrawBuildPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_Survivors",   ctx.GetBuffer(data.m_hSurvivors)->GetDefaultView(xiiGALBufferViewType::ShaderResource),     xiiGALShaderType::Compute);
  cmd.ResolveAndSetShaderResourceBufferView("g_InstanceLOD", ctx.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource),    xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawArgs",   ctx.GetBuffer(data.m_hDrawCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),  xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawCounts", ctx.GetBuffer(data.m_hDrawCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),    xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Shadow caster list build
// 
namespace
{
  struct ShadowCasterBuildData
  {
    xiiRGBufferHandle m_hVisibleCandidates;
    xiiRGBufferHandle m_hShadowCasterCommands;
  };
}

static void SetupShadowCasterBuild(xiiView& view, ShadowCasterBuildData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hCandidates;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);
  data.m_hVisibleCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 20u; // DrawIndexedIndirectArguments per cascade-per-bin
  desc.m_uiSize              = desc.m_uiElementByteStride * k_uiMaxMaterialBins * 4u; // 4 cascades
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  desc.m_Mode                = xiiGALBufferMode::Formatted;
  data.m_hShadowCasterCommands = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pShadowCasterBuildPipeline, "Shaders/Pipeline/ShadowCasterCulling.xiiShader");
}

static void ExecuteShadowCasterBuild(xiiView& view, const ShadowCasterBuildData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("ShadowCasterListBuild");
  cmd.SetPipelineState(vp.m_pShadowCasterBuildPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", ctx.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),       xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawArgs",   ctx.GetBuffer(data.m_hShadowCasterCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(k_uiMaxInstances + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Cluster grid build
// 
namespace
{
  struct ClusterBuildData
  {
    xiiRGBufferHandle m_hClusterDescriptors;
    xiiUInt32         m_uiClusterX = 16u, m_uiClusterY = 9u, m_uiClusterZ = 24u;
    float             m_fNearPlane = 0.1f, m_fFarPlane = 1000.0f;
  };
}

static void SetupClusterBuild(xiiView& view, ClusterBuildData& data, xiiRGBuilder& builder)
{
  auto& vp          = view.m_ViewPassResources.m_VisibilityPasses;
  data.m_uiClusterX = static_cast<xiiUInt32>(cvar_ClusterX.GetValue());
  data.m_uiClusterY = static_cast<xiiUInt32>(cvar_ClusterY.GetValue());
  data.m_uiClusterZ = static_cast<xiiUInt32>(cvar_ClusterZ.GetValue());

  if (const xiiCamera* cam = view.GetCamera())
  {
    data.m_fNearPlane = cam->GetNearPlane();
    data.m_fFarPlane  = cam->GetFarPlane();
  }

  const xiiUInt32 uiTotalClusters = data.m_uiClusterX * data.m_uiClusterY * data.m_uiClusterZ;

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 32u;  // float4 min + float4 max per cluster AABB
  desc.m_uiSize              = desc.m_uiElementByteStride * uiTotalClusters;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hClusterDescriptors = builder.WriteBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pClusterBuildPipeline, "Shaders/Pipeline/ClusterGridBuild.xiiShader");
}

static void ExecuteClusterBuild(xiiView& view, const ClusterBuildData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("ClusterGridBuild");
  cmd.SetPipelineState(vp.m_pClusterBuildPipeline);
  // Constant buffer with clustering params (uploaded inline via map).
  // TODO: upload xiiLightClusteringConstants here.
  cmd.ResolveAndSetUnorderedAccessBufferView("g_ClustersOut", ctx.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

  const xiiUInt32 uiTotal  = data.m_uiClusterX * data.m_uiClusterY * data.m_uiClusterZ;
  const xiiUInt32 uiGroups = (uiTotal + 63u) / 64u;
  cmd.DispatchCompute({uiGroups, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Light list build
// 
namespace
{
  struct LightListData
  {
    xiiRGBufferHandle m_hClusterDescriptors;
    xiiRGBufferHandle m_hLightIndexBuffer;
    xiiRGBufferHandle m_hLightGridBuffer;
    xiiUInt32         m_uiActiveLightCount = 0;
  };
}

static void SetupLightListBuild(xiiView& view, LightListData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hClusters;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_ClusterDescriptors), hClusters);
  data.m_hClusterDescriptors = builder.ReadBuffer(hClusters, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiMaxClusters     = 16u * 9u * 24u; // worst case
  const xiiUInt32 uiMaxLightsPerCl  = 256u;

  xiiGALBufferCreationDescription idxDesc;
  idxDesc.m_uiElementByteStride = 4u;
  idxDesc.m_uiSize              = 4u * uiMaxClusters * uiMaxLightsPerCl;
  idxDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  idxDesc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightIndexBuffer      = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, idxDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription gridDesc;
  gridDesc.m_uiElementByteStride = 8u;  // uint2 (offset, count) per cluster
  gridDesc.m_uiSize              = gridDesc.m_uiElementByteStride * uiMaxClusters;
  gridDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  gridDesc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightGridBuffer        = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, gridDesc, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiActiveLightCount = xiiClusteredDataCPU::MAX_LIGHT_DATA;

  xiiView::EnsureComputePipeline(vp.m_pLightListPipeline, "Shaders/Pipeline/LightListBuild.xiiShader");
}

static void ExecuteLightListBuild(xiiView& view, const LightListData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("LightListBuild");
  cmd.SetPipelineState(vp.m_pLightListPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_Clusters",     ctx.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource),   xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex",  ctx.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),    xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid",   ctx.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),     xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

  const xiiUInt32 uiGroups = (data.m_uiActiveLightCount + 63u) / 64u;
  cmd.DispatchCompute({uiGroups, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Reflection probe selection
// 
namespace
{
  struct ReflProbeSelectData
  {
    xiiRGBufferHandle m_hClusterDescriptors;
    xiiRGBufferHandle m_hProbeMask;
  };
}

static void SetupReflProbeSelect(xiiView& view, ReflProbeSelectData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  xiiRGBufferHandle hClusters;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_ClusterDescriptors), hClusters);
  data.m_hClusterDescriptors = builder.ReadBuffer(hClusters, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 4u;
  desc.m_uiSize              = 4u * xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hProbeMask          = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pProbeSelectPipeline, "Shaders/Pipeline/GpuDrivenVisibilityCulling.xiiShader");
}

static void ExecuteReflProbeSelect(xiiView& view, const ReflProbeSelectData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("ReflectionProbeSelection");
  cmd.SetPipelineState(vp.m_pProbeSelectPipeline);
  cmd.ResolveAndSetShaderResourceBufferView("g_Clusters",   ctx.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_ProbeMask", ctx.GetBuffer(data.m_hProbeMask)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),         xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

// 
// Volumetric grid allocation (froxel setup)
// 
namespace
{
  struct FroxelAllocData
  {
    xiiRGBufferHandle  m_hFroxelMetadata;
    xiiRGTextureHandle m_hFroxelScattering;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupFroxelAlloc(xiiView& view, FroxelAllocData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiGALBufferCreationDescription metaDesc;
  metaDesc.m_uiElementByteStride = 32u; // per-froxel density + phase + absorption
  metaDesc.m_uiSize              = metaDesc.m_uiElementByteStride * 128u * 72u * 64u; // froxel volume
  metaDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  metaDesc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hFroxelMetadata         = builder.WriteBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, metaDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription scatDesc;
  scatDesc.m_TextureType = xiiGALTextureType::Texture3D;
  scatDesc.m_Format      = xiiGALTextureFormat::RGBA16Float;
  scatDesc.m_uiWidth     = 128u;
  scatDesc.m_uiHeight    = 72u;
  scatDesc.m_uiDepth     = 64u;
  scatDesc.m_uiMipLevels = 1u;
  scatDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  scatDesc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hFroxelScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, scatDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pFroxelSetupPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

static void ExecuteFroxelAlloc(xiiView& view, const FroxelAllocData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& vp               = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("VolumetricGridAlloc");
  cmd.SetPipelineState(vp.m_pFroxelSetupPipeline);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetaOut",     ctx.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),      xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_FroxelScatteringOut",     ctx.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(128u + 7u) / 8u, (72u + 7u) / 8u, 8u});
  cmd.EndDebugGroup();
}

// 
// BuildStage1_Visibility - entry point called from BuildDefaultRenderGraph
// 

void xiiView::BuildStage1_Visibility(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this; // captured by [this] lambdas

  // 1a. GPU occlusion readback (rotates ring buffer, side-effects = true)
  graph.AddPass<OcclusionReadbackData>(
    "GpuOcclusionReadback",
    xiiGALCommandQueueFlags::Compute,
    [self](OcclusionReadbackData& data, xiiRGBuilder& b) { SetupOcclusionReadback(*self, data, b); },
    [self](const OcclusionReadbackData& data, xiiRGPassContext& c) { ExecuteOcclusionReadback(*self, data, c); });

  // 1b. Frustum culling
  graph.AddPass<FrustumCullData>(
    "FrustumCulling",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](FrustumCullData& data, xiiRGBuilder& b) { SetupFrustumCulling(*self, data, b); },
    [self](const FrustumCullData& data, xiiRGPassContext& c) { ExecuteFrustumCulling(*self, data, c); });

  // 1c. LOD selection
  graph.AddPass<LODSelectData>(
    "LODSelection",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](LODSelectData& data, xiiRGBuilder& b) { SetupLODSelection(*self, data, b, blackboard); },
    [self](const LODSelectData& data, xiiRGPassContext& c) { ExecuteLODSelection(*self, data, c); });

  // 1d. Instance update (world matrices & bounds)
  graph.AddPass<InstanceUpdateData>(
    "InstanceUpdate",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](InstanceUpdateData& data, xiiRGBuilder& b) { SetupInstanceUpdate(*self, data, b, blackboard); },
    [self](const InstanceUpdateData& data, xiiRGPassContext& c) { ExecuteInstanceUpdate(*self, data, c); });

  // 1e. Draw command build (GPU-driven indirect args)
  graph.AddPass<DrawBuildData>(
    "DrawCommandBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](DrawBuildData& data, xiiRGBuilder& b) { SetupDrawBuild(*self, data, b, blackboard); },
    [self](const DrawBuildData& data, xiiRGPassContext& c) { ExecuteDrawBuild(*self, data, c); });

  // 1f. Shadow caster list build
  graph.AddPass<ShadowCasterBuildData>(
    "ShadowCasterListBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ShadowCasterBuildData& data, xiiRGBuilder& b) { SetupShadowCasterBuild(*self, data, b, blackboard); },
    [self](const ShadowCasterBuildData& data, xiiRGPassContext& c) { ExecuteShadowCasterBuild(*self, data, c); });

  // 1g. Cluster grid build
  graph.AddPass<ClusterBuildData>(
    "ClusterGridBuild",
    xiiGALCommandQueueFlags::Compute,
    [self](ClusterBuildData& data, xiiRGBuilder& b) { SetupClusterBuild(*self, data, b); },
    [self](const ClusterBuildData& data, xiiRGPassContext& c) { ExecuteClusterBuild(*self, data, c); });

  // 1h. Light list build
  graph.AddPass<LightListData>(
    "LightListBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](LightListData& data, xiiRGBuilder& b) { SetupLightListBuild(*self, data, b, blackboard); },
    [self](const LightListData& data, xiiRGPassContext& c) { ExecuteLightListBuild(*self, data, c); });

  // 1i. Reflection probe selection
  graph.AddPass<ReflProbeSelectData>(
    "ReflectionProbeSelection",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ReflProbeSelectData& data, xiiRGBuilder& b) { SetupReflProbeSelect(*self, data, b, blackboard); },
    [self](const ReflProbeSelectData& data, xiiRGPassContext& c) { ExecuteReflProbeSelect(*self, data, c); });

  // 1j. Volumetric froxel grid allocation
  graph.AddPass<FroxelAllocData>(
    "VolumetricGridAlloc",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](FroxelAllocData& data, xiiRGBuilder& b) { SetupFroxelAlloc(*self, data, b, blackboard); },
    [self](const FroxelAllocData& data, xiiRGPassContext& c) { ExecuteFroxelAlloc(*self, data, c); });
}
