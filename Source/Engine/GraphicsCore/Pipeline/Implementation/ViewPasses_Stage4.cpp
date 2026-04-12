// XII Engine - ViewPasses_Stage4.cpp
// Stage 4: G-Buffer generation (graphics - native render pass with merged sub-passes).
// GBufferBase: albedo / normal / material / emissive -> merged with depth from Stage 3.
// NormalRoughnessPrepass: compact normal+roughness for GTAO early access.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

//
// GBuffer base pass (graphics - writes 4 MRT + reuses depth)
//
namespace
{
  struct GBufferBaseData
  {
    xiiRGTextureHandle m_hSceneDepth; // read+write (depth test, no depth write)
    xiiRGTextureHandle m_hAlbedo;
    xiiRGTextureHandle m_hNormal;
    xiiRGTextureHandle m_hMaterial;
    xiiRGTextureHandle m_hEmissive;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupGBufferBase(xiiView& view, GBufferBaseData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& gbp = view.m_ViewPassResources.m_GBufferPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  // Depth: read-only depth test (already written by Stage 3 DepthPrepass).
  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);

  // Read indirect draw commands.
  xiiRGBufferHandle hDrawCmds;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawCmds);
  if (hDrawCmds.IsValid())
    data.m_hDrawCommands = builder.ReadBuffer(hDrawCmds, xiiGALResourceStateFlags::IndirectArgument);

  // GBuffer render targets.
  auto makeRT = [&](xiiGALTextureFormat fmt, const char* szKey) -> xiiRGTextureHandle {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType = xiiGALTextureType::Texture2D;
    desc.m_Format      = fmt;
    desc.m_uiWidth     = data.m_uiRenderW;
    desc.m_uiHeight    = data.m_uiRenderH;
    desc.m_uiMipLevels = 1u;
    desc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    desc.m_Usage       = xiiGALResourceUsage::Default;
    return builder.WriteTexture(szKey, desc, xiiGALResourceStateFlags::RenderTarget);
  };

  data.m_hAlbedo   = makeRT(xiiGALTextureFormat::RGBA8Unorm, xiiRGBlackboardKeys::k_GBufferAlbedo);
  data.m_hNormal   = makeRT(xiiGALTextureFormat::RG16SNorm, xiiRGBlackboardKeys::k_GBufferNormal);
  data.m_hMaterial = makeRT(xiiGALTextureFormat::RGBA8Unorm, xiiRGBlackboardKeys::k_GBufferMaterial);
  data.m_hEmissive = makeRT(xiiGALTextureFormat::RGBA16Float, xiiRGBlackboardKeys::k_GBufferEmissive);

  builder.SetPassAllowMerge(true); // Flag for native render-pass merging with Stage 3 depth.
}

static void ExecuteGBufferBase(xiiView& view, const GBufferBaseData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              gbp = view.m_ViewPassResources.m_GBufferPasses;

  cmd.BeginDebugGroup("GBufferBase");

  // Clear MRTs.
  const float fBlackAlpha[4]    = {0.0f, 0.0f, 0.0f, 1.0f};
  const float fDefaultNormal[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // oct-encoded zero
  const float fDefaultMat[4]    = {0.5f, 0.0f, 1.0f, 0.0f}; // roughness=0.5, metallic=0, AO=1
  const float fBlackEmissive[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hAlbedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fBlackAlpha, xiiGALStateTransitionMode::Transition);
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hNormal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fDefaultNormal, xiiGALStateTransitionMode::None);
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hMaterial)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fDefaultMat, xiiGALStateTransitionMode::None);
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hEmissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fBlackEmissive, xiiGALStateTransitionMode::None);

  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}},
                   data.m_uiRenderW, data.m_uiRenderH);

  if (gbp.m_pGBufferPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(gbp.m_pGBufferPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }

  cmd.EndDebugGroup();
}

//
// Normal + roughness prepass (graphics - compact R8G8B8A8 buffer for GTAO)
//
namespace
{
  struct NormalRoughnessData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hNormalRoughness;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupNormalRoughness(xiiView& view, NormalRoughnessData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& gbp = view.m_ViewPassResources.m_GBufferPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);

  xiiRGBufferHandle hDraw;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDraw);
  if (hDraw.IsValid())
    data.m_hDrawCommands = builder.ReadBuffer(hDraw, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType      = xiiGALTextureType::Texture2D;
  desc.m_Format           = xiiGALTextureFormat::RGBA8Unorm; // r/g = oct normal, b = roughness, a = specular
  desc.m_uiWidth          = data.m_uiRenderW;
  desc.m_uiHeight         = data.m_uiRenderH;
  desc.m_uiMipLevels      = 1u;
  desc.m_BindFlags        = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  desc.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hNormalRoughness = builder.WriteTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, desc, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

static void ExecuteNormalRoughness(xiiView& view, const NormalRoughnessData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              gbp = view.m_ViewPassResources.m_GBufferPasses;

  cmd.BeginDebugGroup("NormalRoughnessPrepass");

  const float fClear[4] = {0.0f, 0.0f, 0.5f, 1.0f}; // mid roughness, full specular default
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fClear, xiiGALStateTransitionMode::Transition);
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}},
                   data.m_uiRenderW, data.m_uiRenderH);

  if (gbp.m_pNormalRoughnessPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(gbp.m_pNormalRoughnessPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }

  cmd.EndDebugGroup();
}

//
// BuildStage4_GBuffer - entry point
//

void xiiView::BuildStage4_GBuffer(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  // 4a. Four-target GBuffer (albedo, normal, material, emissive) + depth read-only test.
  graph.AddPass<GBufferBaseData>(
    "GBufferBase",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](GBufferBaseData& d, xiiRGBuilder& b) { SetupGBufferBase(*self, d, b, blackboard); },
    [self](const GBufferBaseData& d, xiiRGPassContext& c) { ExecuteGBufferBase(*self, d, c); });

  // 4b. Compact normal+roughness prepass (used by GTAO in Stage 5).
  graph.AddPass<NormalRoughnessData>(
    "NormalRoughnessPrepass",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](NormalRoughnessData& d, xiiRGBuilder& b) { SetupNormalRoughness(*self, d, b, blackboard); },
    [self](const NormalRoughnessData& d, xiiRGPassContext& c) { ExecuteNormalRoughness(*self, d, c); });
}
