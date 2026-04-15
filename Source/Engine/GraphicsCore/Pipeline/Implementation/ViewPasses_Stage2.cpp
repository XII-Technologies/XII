// XII Engine - ViewPasses_Stage2.cpp
// Stage 2: Shadow map generation.
// Cascade setup (compute) -> directional shadows (graphics) -> spot/point shadows (graphics) ->
// ray-traced shadows (RT compute) -> shadow denoise (compute) -> contact shadows (compute).

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

//
// Spot light shadow maps (graphics)
//
namespace
{
  struct SpotShadowData
  {
    xiiRGBufferHandle  m_hShadowCasterCommands;
    xiiRGTextureHandle m_hLocalShadowAtlas;
    xiiUInt32          m_uiSpotLightCount = 0;
  };
} // namespace

static void SetupSpotShadow(xiiView& view, SpotShadowData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  if (!sp.m_pLocalShadowAtlas)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType     = xiiGALTextureType::Texture2D;
    desc.m_Format          = xiiGALTextureFormat::D32Float;
    desc.m_uiWidth         = k_uiLocalAtlasSize;
    desc.m_uiHeight        = k_uiLocalAtlasSize;
    desc.m_uiMipLevels     = 1u;
    desc.m_BindFlags       = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    desc.m_Usage           = xiiGALResourceUsage::Default;
    sp.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
  }

  xiiRGBufferHandle hCasters;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawShadowCasterCommands), hCasters);
  data.m_hShadowCasterCommands = builder.ReadBuffer(hCasters, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlas     = builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, sp.m_pLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas     = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  // Count spot lights from extracted data.
  if (const xiiExtractedRenderData* pExt = view.GetExtractedRenderData())
  {
    data.m_uiSpotLightCount = static_cast<xiiUInt32>(pExt->GetRenderData(xiiDefaultRenderDataCategories::Light).GetCount());
  }
  builder.SetPassAllowMerge(false);
}

static void ExecuteSpotShadow(xiiView& view, const SpotShadowData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  if (data.m_uiSpotLightCount == 0u || !sp.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("SpotLightShadows");
  xiiGALTexture* pAtlas = ctx.GetTexture(data.m_hLocalShadowAtlas);

  // For each spot light, render into its atlas tile.
  // Atlas allocation managed by LocalLightShadowAtlasAllocation pass (deferred to full impl).
  cmd.ClearDepthStencil(pAtlas->GetDefaultView(xiiGALTextureViewType::DepthStencil), xiiGALClearValueFlags::Depth, 0.0f, 0u, xiiGALStateTransitionMode::Transition);
  cmd.SetPipelineState(sp.m_pShadowDepthPipeline);
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(k_uiLocalAtlasSize), static_cast<float>(k_uiLocalAtlasSize), 0.0f, 1.0f}}, k_uiLocalAtlasSize, k_uiLocalAtlasSize);
  // Indirect multi-draw from shadow caster arg buffer.
  cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hShadowCasterCommands), 0u);
  cmd.EndDebugGroup();
}

//
// Point light cube-face shadow maps (graphics, 6 faces per light)
//
namespace
{
  struct PointShadowData
  {
    xiiRGBufferHandle  m_hShadowCasterCommands;
    xiiRGTextureHandle m_hLocalShadowAtlas; // same atlas as spot lights, different tiles
    xiiUInt32          m_uiPointLightCount = 0;
  };
} // namespace

static void SetupPointShadow(xiiView& view, PointShadowData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  xiiRGBufferHandle hCasters;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawShadowCasterCommands), hCasters);
  data.m_hShadowCasterCommands = builder.ReadBuffer(hCasters, xiiGALResourceStateFlags::IndirectArgument);

  xiiRGTextureHandle hAtlas;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_LocalShadowAtlas), hAtlas);
  data.m_hLocalShadowAtlas = builder.WriteTexture(hAtlas, xiiGALResourceStateFlags::DepthWrite);

  if (const xiiExtractedRenderData* pExt = view.GetExtractedRenderData())
    data.m_uiPointLightCount = static_cast<xiiUInt32>(pExt->GetRenderData(xiiDefaultRenderDataCategories::Light).GetCount());

  builder.SetPassAllowMerge(false);
}

static void ExecutePointShadow(xiiView& view, const PointShadowData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  if (data.m_uiPointLightCount == 0u || !sp.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("PointLightCubeShadows");
  cmd.SetPipelineState(sp.m_pShadowDepthPipeline);
  // Each point light: 6 draw calls placing results into 6 atlas tiles.
  for (xiiUInt32 uiFace = 0; uiFace < data.m_uiPointLightCount * 6u; ++uiFace)
  {
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hShadowCasterCommands), uiFace * 20u);
  }
  cmd.EndDebugGroup();
}

//
// Ray-traced shadows
//
namespace
{
  struct RTShadowData
  {
    xiiRGTextureHandle m_hRTRawShadowMask;
    xiiRGTextureHandle m_hSceneDepth;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupRTShadow(xiiView& view, RTShadowData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  // Depth as input (written by depth prepass = Stage 3, but RT shadows are dispatched after HiZ
  // and before deferred lighting, so the dependency is correct: Stage 3 -> Stage 2 RT shadow
  // is handled by the graph compiler via resource version tracking).
  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  if (hDepth.IsValid())
    data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType      = xiiGALTextureType::Texture2D;
  desc.m_Format           = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth          = data.m_uiRenderW;
  desc.m_uiHeight         = data.m_uiRenderH;
  desc.m_uiMipLevels      = 1u;
  desc.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hRTRawShadowMask = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(sp.m_pShadowDenoisePipeline, "Shaders/Pipeline/RTShadow.xiiShader");
  builder.SetPassAllowMerge(false);
}

static void ExecuteRTShadow(xiiView& view, const RTShadowData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  cmd.BeginDebugGroup("RTShadows");
  cmd.SetPipelineState(sp.m_pShadowDenoisePipeline); // reusing slot for RT pipeline
  if (data.m_hSceneDepth.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_SceneDepth", ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_RTShadowOut", ctx.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Shadow denoiser (bilateral blur on raw RT shadow mask)
//
namespace
{
  struct ShadowDenoiseData
  {
    xiiRGTextureHandle m_hRTRawShadowMask;
    xiiRGTextureHandle m_hRTFinalShadowMask;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupShadowDenoise(xiiView& view, ShadowDenoiseData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hRaw;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawShadowMask), hRaw);
  data.m_hRTRawShadowMask = builder.ReadTexture(hRaw, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType        = xiiGALTextureType::Texture2D;
  desc.m_Format             = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth            = data.m_uiRenderW;
  desc.m_uiHeight           = data.m_uiRenderH;
  desc.m_uiMipLevels        = 1u;
  desc.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hRTFinalShadowMask = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(sp.m_pShadowDenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

static void ExecuteShadowDenoise(xiiView& view, const ShadowDenoiseData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  cmd.BeginDebugGroup("ShadowDenoise");
  cmd.SetPipelineState(sp.m_pShadowDenoisePipeline);
  cmd.ResolveAndSetShaderResourceView("g_Input", ctx.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_Output", ctx.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Contact shadows (screen-space ray march)
//
namespace
{
  struct ContactShadowData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hContactShadow;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupContactShadow(xiiView& view, ContactShadowData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  if (hDepth.IsValid())
    data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType    = xiiGALTextureType::Texture2D;
  desc.m_Format         = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth        = data.m_uiRenderW;
  desc.m_uiHeight       = data.m_uiRenderH;
  desc.m_uiMipLevels    = 1u;
  desc.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hContactShadow = builder.WriteTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(sp.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

static void ExecuteContactShadow(xiiView& view, const ContactShadowData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  cmd.BeginDebugGroup("ContactShadows");
  cmd.SetPipelineState(sp.m_pContactShadowPipeline);
  if (data.m_hSceneDepth.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_SceneDepth", ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_ContactShadowOut", ctx.GetTexture(data.m_hContactShadow)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// BuildStage2_Shadows - entry point called from BuildDefaultRenderGraph
//

void xiiView::BuildStage2_Shadows(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  // 2a. Compute cascade matrices on GPU.
  graph.AddPass<ShadowCascadeSetupData>(
    "ShadowCascadeSetup",
    xiiGALCommandQueueFlags::Compute,
    [self](ShadowCascadeSetupData& d, xiiRGBuilder& b) { SetupShadowCascadeSetup(*self, d, b); },
    [self](const ShadowCascadeSetupData& d, xiiRGPassContext& c) { ExecuteShadowCascadeSetup(*self, d, c); });

  // 2b. Directional cascaded shadow maps.
  graph.AddPass<DirShadowData>(
    "DirectionalShadowMap",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](DirShadowData& d, xiiRGBuilder& b) { SetupDirShadow(*self, d, b, blackboard); },
    [self](const DirShadowData& d, xiiRGPassContext& c) { ExecuteDirShadow(*self, d, c); });

  // 2c. Spot light shadow maps.
  graph.AddPass<SpotShadowData>(
    "SpotLightShadow",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](SpotShadowData& d, xiiRGBuilder& b) { SetupSpotShadow(*self, d, b, blackboard); },
    [self](const SpotShadowData& d, xiiRGPassContext& c) { ExecuteSpotShadow(*self, d, c); });

  // 2d. Point light cube shadow maps.
  graph.AddPass<PointShadowData>(
    "PointLightShadow",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](PointShadowData& d, xiiRGBuilder& b) { SetupPointShadow(*self, d, b, blackboard); },
    [self](const PointShadowData& d, xiiRGPassContext& c) { ExecutePointShadow(*self, d, c); });

  // 2e. Ray-traced shadows (runs after depth prepass feeds the depth buffer).
  graph.AddPass<RTShadowData>(
    "RTShadow",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](RTShadowData& d, xiiRGBuilder& b) { SetupRTShadow(*self, d, b, blackboard); },
    [self](const RTShadowData& d, xiiRGPassContext& c) { ExecuteRTShadow(*self, d, c); });

  // 2f. Shadow denoiser / bilateral blur.
  graph.AddPass<ShadowDenoiseData>(
    "ShadowDenoise",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ShadowDenoiseData& d, xiiRGBuilder& b) { SetupShadowDenoise(*self, d, b, blackboard); },
    [self](const ShadowDenoiseData& d, xiiRGPassContext& c) { ExecuteShadowDenoise(*self, d, c); });

  // 2g. Contact shadows.
  graph.AddPass<ContactShadowData>(
    "ContactShadows",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ContactShadowData& d, xiiRGBuilder& b) { SetupContactShadow(*self, d, b, blackboard); },
    [self](const ContactShadowData& d, xiiRGPassContext& c) { ExecuteContactShadow(*self, d, c); });
}
