// XII Engine - ViewPasses_Stage1.cpp
// Stage 1: Frame setup & visibility passes.
// All passes on the Compute queue. Frustum cull -> LOD -> instance update -> draw-command build ->
// shadow-caster list -> cluster grid -> light list -> reflection probe selection -> froxel grid alloc.

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
} // namespace

static void SetupFroxelAlloc(xiiView& view, FroxelAllocData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& vp = view.m_ViewPassResources.m_VisibilityPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiGALBufferCreationDescription metaDesc;
  metaDesc.m_uiElementByteStride = 32u;                                               // per-froxel density + phase + absorption
  metaDesc.m_uiSize              = metaDesc.m_uiElementByteStride * 128u * 72u * 64u; // froxel volume
  metaDesc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  metaDesc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hFroxelMetadata         = builder.WriteBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, metaDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription scatDesc;
  scatDesc.m_TextureType   = xiiGALTextureType::Texture3D;
  scatDesc.m_Format        = xiiGALTextureFormat::RGBA16Float;
  scatDesc.m_uiWidth       = 128u;
  scatDesc.m_uiHeight      = 72u;
  scatDesc.m_uiDepth       = 64u;
  scatDesc.m_uiMipLevels   = 1u;
  scatDesc.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  scatDesc.m_Usage         = xiiGALResourceUsage::Default;
  data.m_hFroxelScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, scatDesc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(vp.m_pFroxelSetupPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

static void ExecuteFroxelAlloc(xiiView& view, const FroxelAllocData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              vp  = view.m_ViewPassResources.m_VisibilityPasses;

  cmd.BeginDebugGroup("VolumetricGridAlloc");
  cmd.SetPipelineState(vp.m_pFroxelSetupPipeline);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetaOut", ctx.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_FroxelScatteringOut", ctx.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
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
