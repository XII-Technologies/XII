// XII Engine - ViewPasses_Stage8_12.cpp
// Stages 8–12: Transparency, Screen-Space, Temporal, Post-Process, Final Output.
// Combined into one file to complete the pipeline.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

//
// Helper macros shared across stages in this file
//
#define READ_TEX(hOut, key)                                                                     \
  do                                                                                            \
  {                                                                                             \
    xiiRGTextureHandle _h;                                                                      \
    bb.TryGetValue(xiiMakeHashedString(key), _h);                                               \
    if (_h.IsValid()) hOut = builder.ReadTexture(_h, xiiGALResourceStateFlags::ShaderResource); \
  } while (false)

#define WRITE_TEX_HDR(hOut, key, w, h)                                                           \
  do                                                                                             \
  {                                                                                              \
    xiiGALTextureCreationDescription _d;                                                         \
    _d.m_TextureType = xiiGALTextureType::Texture2D;                                             \
    _d.m_Format      = xiiGALTextureFormat::RGBA16Float;                                         \
    _d.m_uiWidth     = (w);                                                                      \
    _d.m_uiHeight    = (h);                                                                      \
    _d.m_uiMipLevels = 1u;                                                                       \
    _d.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;       \
    _d.m_Usage       = xiiGALResourceUsage::Default;                                             \
    hOut             = builder.WriteTexture(key, _d, xiiGALResourceStateFlags::UnorderedAccess); \
  } while (false)

#define WRITE_TEX_R8(hOut, key, w, h)                                                            \
  do                                                                                             \
  {                                                                                              \
    xiiGALTextureCreationDescription _d;                                                         \
    _d.m_TextureType = xiiGALTextureType::Texture2D;                                             \
    _d.m_Format      = xiiGALTextureFormat::R8Unorm;                                             \
    _d.m_uiWidth     = (w);                                                                      \
    _d.m_uiHeight    = (h);                                                                      \
    _d.m_uiMipLevels = 1u;                                                                       \
    _d.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;       \
    _d.m_Usage       = xiiGALResourceUsage::Default;                                             \
    hOut             = builder.WriteTexture(key, _d, xiiGALResourceStateFlags::UnorderedAccess); \
  } while (false)

#define BIND_SRV_TEX(slot, h)                                                                                                                                          \
  do                                                                                                                                                                   \
  {                                                                                                                                                                    \
    if ((h).IsValid()) cmd.ResolveAndSetShaderResourceView(slot, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute); \
  } while (false)

#define BIND_SRV_BUF(slot, h)                                                                                                                                              \
  do                                                                                                                                                                       \
  {                                                                                                                                                                        \
    if ((h).IsValid()) cmd.ResolveAndSetShaderResourceBufferView(slot, ctx.GetBuffer(h)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute); \
  } while (false)

#define BIND_UAV_TEX(slot, h)                                                                                                                                            \
  do                                                                                                                                                                     \
  {                                                                                                                                                                      \
    if ((h).IsValid()) cmd.ResolveAndSetUnorderedAccessView(slot, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute); \
  } while (false)

#define DISPATCH(W, H) cmd.DispatchCompute({((W) + 7u) / 8u, ((H) + 7u) / 8u, 1u})

// ═════════════════════════════════════════════════════════════════════════════
// STAGE 8 - Transparency & special materials
// ═════════════════════════════════════════════════════════════════════════════

//  GPU particle simulation
namespace
{
  struct ParticleSimData
  {
    xiiRGBufferHandle m_hParticleState;
    xiiUInt32         m_uiParticleCount = 0;
  };
} // namespace

static void SetupParticleSim(xiiView& view, ParticleSimData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto&           tp    = view.m_ViewPassResources.m_TransparencyPasses;
  const xiiUInt32 uiCap = 65536u;

  if (!tp.m_pParticleStateBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = 64u; // position(3) + velocity(3) + age + lifetime + color(4) + size + pad(3)
    desc.m_uiSize              = desc.m_uiElementByteStride * uiCap;
    desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Mode                = xiiGALBufferMode::Structured;
    tp.m_pParticleStateBuffer  = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    tp.m_uiParticleCapacity    = uiCap;
  }

  data.m_hParticleState  = builder.ImportBuffer("ParticleState", tp.m_pParticleStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hParticleState  = builder.WriteBuffer(data.m_hParticleState, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiParticleCount = uiCap;

  xiiView::EnsureComputePipeline(tp.m_pParticleSimulatePipeline, "Shaders/Pipeline/GPUParticleSimulate.xiiShader");
}

static void ExecuteParticleSim(xiiView& view, const ParticleSimData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              tp  = view.m_ViewPassResources.m_TransparencyPasses;
  cmd.BeginDebugGroup("GPUParticleSimulate");
  cmd.SetPipelineState(tp.m_pParticleSimulatePipeline);
  BIND_UAV_TEX("g_Particles", data.m_hParticleState); // intentional buffer → UAV via buffer view
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiParticleCount + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

//  Screen-space decal classify + resolve
namespace
{
  struct SSDecalData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hGBufAlbedo, m_hGBufNormal, m_hGBufMaterial;
    xiiRGBufferHandle  m_hDecalTileList;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupSSDecal(xiiView& view, SSDecalData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TransparencyPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);
  READ_TEX(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  READ_TEX(data.m_hGBufAlbedo, xiiRGBlackboardKeys::k_GBufferAlbedo);
  READ_TEX(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  READ_TEX(data.m_hGBufMaterial, xiiRGBlackboardKeys::k_GBufferMaterial);

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 4u;
  desc.m_uiSize              = 4u * 1024u * 16u;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hDecalTileList      = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalTileList, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(tp.m_pSSDecalClassifyPipeline, "Shaders/Pipeline/DecalClassification.xiiShader");
  xiiView::EnsureComputePipeline(tp.m_pSSDecalResolvePipeline, "Shaders/Pipeline/DecalResolve.xiiShader");
}

static void ExecuteSSDecal(xiiView& view, const SSDecalData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              tp  = view.m_ViewPassResources.m_TransparencyPasses;

  // Classify.
  cmd.BeginDebugGroup("ScreenSpaceDecals");
  cmd.SetPipelineState(tp.m_pSSDecalClassifyPipeline);
  BIND_SRV_TEX("g_SceneDepth", data.m_hSceneDepth);
  BIND_UAV_TEX("g_TileList", data.m_hDecalTileList);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_uiRenderW, data.m_uiRenderH);

  // Resolve.
  cmd.SetPipelineState(tp.m_pSSDecalResolvePipeline);
  BIND_SRV_TEX("g_SceneDepth", data.m_hSceneDepth);
  BIND_SRV_BUF("g_TileList", data.m_hDecalTileList);
  BIND_SRV_TEX("g_GBufAlbedo", data.m_hGBufAlbedo);
  BIND_SRV_TEX("g_GBufNormal", data.m_hGBufNormal);
  BIND_SRV_TEX("g_GBufMat", data.m_hGBufMaterial);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_uiRenderW, data.m_uiRenderH);
  cmd.EndDebugGroup();
}

//  WBOIT accumulate + reveal targets and resolve
namespace
{
  struct OITData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth;
    xiiRGTextureHandle m_hOITAccum, m_hOITReveal;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupOIT(xiiView& view, OITData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TransparencyPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle h;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), h);
  if (h.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(h, xiiGALResourceStateFlags::RenderTarget);
  xiiRGTextureHandle hD;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hD);
  if (hD.IsValid()) data.m_hSceneDepth = builder.ReadTexture(hD, xiiGALResourceStateFlags::DepthRead);

  WRITE_TEX_HDR(data.m_hOITAccum, xiiRGBlackboardKeys::k_OITAccumulateBuffer, data.m_uiRenderW, data.m_uiRenderH);
  WRITE_TEX_R8(data.m_hOITReveal, xiiRGBlackboardKeys::k_OITRevealBuffer, data.m_uiRenderW, data.m_uiRenderH);

  xiiRGBufferHandle hC;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hC);
  if (hC.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(hC, xiiGALResourceStateFlags::IndirectArgument);

  xiiView::EnsureComputePipeline(tp.m_pOITResolvePipeline, "Shaders/Pipeline/OITResolve.xiiShader");
  builder.SetPassAllowMerge(false);
}

static void ExecuteOIT(xiiView& view, const OITData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              tp  = view.m_ViewPassResources.m_TransparencyPasses;

  // 1. Render translucents into accum+reveal via blended graphics pipeline.
  cmd.BeginDebugGroup("WeightedBlendedOIT");
  const float fZero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  const float fOne[4]  = {1.0f, 1.0f, 1.0f, 1.0f};
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hOITAccum)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fZero, xiiGALStateTransitionMode::Transition);
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hOITReveal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fOne, xiiGALStateTransitionMode::None);
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);
  if (tp.m_pTranslucentPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(tp.m_pTranslucentPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }

  // 2. Resolve WBOIT into HDR target via compute.
  cmd.SetPipelineState(tp.m_pOITResolvePipeline);
  BIND_SRV_TEX("g_OITAccum", data.m_hOITAccum);
  BIND_SRV_TEX("g_OITReveal", data.m_hOITReveal);
  BIND_UAV_TEX("g_HDROut", data.m_hHDRSceneColor);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_uiRenderW, data.m_uiRenderH);
  cmd.EndDebugGroup();
}

void xiiView::BuildStage8_Transparency(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView*                        self = this;
  const xiiRenderGraphBlackboard& bb   = blackboard;

  graph.AddPass<ParticleSimData>(
    "GPUParticleSimulate", xiiGALCommandQueueFlags::Compute,
    [self, &bb](ParticleSimData& d, xiiRGBuilder& b) { SetupParticleSim(*self, d, b, bb); },
    [self](const ParticleSimData& d, xiiRGPassContext& c) { ExecuteParticleSim(*self, d, c); });

  graph.AddPass<SSDecalData>(
    "ScreenSpaceDecals", xiiGALCommandQueueFlags::Compute,
    [self, &bb](SSDecalData& d, xiiRGBuilder& b) { SetupSSDecal(*self, d, b, bb); },
    [self](const SSDecalData& d, xiiRGPassContext& c) { ExecuteSSDecal(*self, d, c); });

  graph.AddPass<OITData>(
    "WeightedBlendedOIT", xiiGALCommandQueueFlags::Graphics,
    [self, &bb](OITData& d, xiiRGBuilder& b) { SetupOIT(*self, d, b, bb); },
    [self](const OITData& d, xiiRGPassContext& c) { ExecuteOIT(*self, d, c); });
}

// ═════════════════════════════════════════════════════════════════════════════
// STAGE 9 - Screen-Space Effects
// ═════════════════════════════════════════════════════════════════════════════
namespace
{
  struct SSGIData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hGBufNormal, m_hHDRIn, m_hSSGIOut;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct SSRefractData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hHDRIn, m_hHDROut, m_hGBufNormal;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct PlanarReflData
  {
    xiiRGTextureHandle m_hPlanarTarget;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
} // namespace

static void SetupSSGI(xiiView& view, SSGIData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ScreenSpacePasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  READ_TEX(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_HDRSceneColor);
  WRITE_TEX_HDR(data.m_hSSGIOut, "SSGITerm", data.m_W, data.m_H);
  xiiView::EnsureComputePipeline(sp.m_pSSGIPipeline, "Shaders/Pipeline/SSGI.xiiShader");
}
static void ExecuteSSGI(xiiView& view, const SSGIData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("SSGI");
  cmd.SetPipelineState(view.m_ViewPassResources.m_ScreenSpacePasses.m_pSSGIPipeline);
  BIND_SRV_TEX("g_SceneDepth", data.m_hSceneDepth);
  BIND_SRV_TEX("g_GBufNormal", data.m_hGBufNormal);
  BIND_SRV_TEX("g_HDRScene", data.m_hHDRIn);
  BIND_UAV_TEX("g_SSGIOut", data.m_hSSGIOut);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupSSRefraction(xiiView& view, SSRefractData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ScreenSpacePasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  READ_TEX(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_HDRSceneColor);
  xiiRGTextureHandle hHDR;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);
  if (hHDR.IsValid()) data.m_hHDROut = builder.WriteTexture(hHDR, xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(sp.m_pSSRefractionPipeline, "Shaders/Pipeline/SSRefraction.xiiShader");
}
static void ExecuteSSRefraction(xiiView& view, const SSRefractData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("SSRefraction");
  cmd.SetPipelineState(view.m_ViewPassResources.m_ScreenSpacePasses.m_pSSRefractionPipeline);
  BIND_SRV_TEX("g_SceneDepth", data.m_hSceneDepth);
  BIND_SRV_TEX("g_GBufNormal", data.m_hGBufNormal);
  BIND_SRV_TEX("g_HDRIn", data.m_hHDRIn);
  BIND_UAV_TEX("g_HDROut", data.m_hHDROut);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupPlanarRefl(xiiView& view, PlanarReflData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ScreenSpacePasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);

  if (!sp.m_pPlanarReflectionTarget)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType           = xiiGALTextureType::Texture2D;
    desc.m_Format                = xiiGALTextureFormat::RGBA16Float;
    desc.m_uiWidth               = data.m_W / 2u;
    desc.m_uiHeight              = data.m_H / 2u;
    desc.m_uiMipLevels           = 1u;
    desc.m_BindFlags             = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    desc.m_Usage                 = xiiGALResourceUsage::Default;
    sp.m_pPlanarReflectionTarget = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
  }
  data.m_hPlanarTarget = builder.ImportTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, sp.m_pPlanarReflectionTarget, xiiGALResourceStateFlags::RenderTarget);
  data.m_hPlanarTarget = builder.WriteTexture(data.m_hPlanarTarget, xiiGALResourceStateFlags::RenderTarget);
  builder.SetPassSideEffects(true);
}
static void ExecutePlanarRefl(xiiView& view, const PlanarReflData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("PlanarReflections");
  const float fClear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hPlanarTarget)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fClear, xiiGALStateTransitionMode::Transition);
  // Secondary view rendering would be dispatched here via xiiRenderWorldModule secondary pass.
  cmd.EndDebugGroup();
}

void xiiView::BuildStage9_ScreenSpace(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView*                        self = this;
  const xiiRenderGraphBlackboard& bb   = blackboard;

  graph.AddPass<SSGIData>("SSGI", xiiGALCommandQueueFlags::Compute, [self, &bb](SSGIData& d, xiiRGBuilder& b) { SetupSSGI(*self, d, b, bb); }, [self](const SSGIData& d, xiiRGPassContext& c) { ExecuteSSGI(*self, d, c); });

  graph.AddPass<SSRefractData>("SSRefraction", xiiGALCommandQueueFlags::Compute, [self, &bb](SSRefractData& d, xiiRGBuilder& b) { SetupSSRefraction(*self, d, b, bb); }, [self](const SSRefractData& d, xiiRGPassContext& c) { ExecuteSSRefraction(*self, d, c); });

  graph.AddPass<PlanarReflData>("PlanarReflections", xiiGALCommandQueueFlags::Graphics, [self, &bb](PlanarReflData& d, xiiRGBuilder& b) { SetupPlanarRefl(*self, d, b, bb); }, [self](const PlanarReflData& d, xiiRGPassContext& c) { ExecutePlanarRefl(*self, d, c); });
}

// ═════════════════════════════════════════════════════════════════════════════
// STAGE 10 - Temporal reconstruction
// ═════════════════════════════════════════════════════════════════════════════
namespace
{
  struct LumHistogramData
  {
    xiiRGTextureHandle m_hHDRIn;
    xiiRGBufferHandle  m_hHistogram;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct AutoExposureData
  {
    xiiRGBufferHandle m_hHistogram, m_hExposure;
  };
  struct TAAData
  {
    xiiRGTextureHandle m_hHDRIn, m_hVelocity, m_hHistory, m_hTAAOut;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct UpscaleData
  {
    xiiRGTextureHandle m_hTAAIn, m_hUpscaled;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct SharpenData
  {
    xiiRGTextureHandle m_hUpscaled, m_hSharpened;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
} // namespace

static void SetupLumHistogram(xiiView& view, LumHistogramData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TemporalPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_HDRSceneColor);
  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 4u;
  desc.m_uiSize              = 4u * 256u;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hHistogram          = builder.WriteBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, desc, xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(tp.m_pLuminanceHistogramPipeline, "Shaders/Pipeline/ExposureHistogram.xiiShader");
}
static void ExecuteLumHistogram(xiiView& view, const LumHistogramData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("LuminanceHistogram");
  cmd.SetPipelineState(view.m_ViewPassResources.m_TemporalPasses.m_pLuminanceHistogramPipeline);
  BIND_SRV_TEX("g_HDRIn", data.m_hHDRIn);
  BIND_UAV_TEX("g_Histogram", data.m_hHistogram);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupAutoExposure(xiiView& view, AutoExposureData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TemporalPasses;

  xiiRGBufferHandle hHist;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_LuminanceHistogram), hHist);
  if (hHist.IsValid()) data.m_hHistogram = builder.ReadBuffer(hHist, xiiGALResourceStateFlags::ShaderResource);

  if (!tp.m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = 4u;
    desc.m_uiSize              = 4u; // single float EV100
    desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Mode                = xiiGALBufferMode::Structured;
    tp.m_pExposureBuffer       = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
  data.m_hExposure = builder.ImportBuffer(xiiRGBlackboardKeys::k_CurrentExposure, tp.m_pExposureBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hExposure = builder.WriteBuffer(data.m_hExposure, xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(tp.m_pAutoExposurePipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader");
}
static void ExecuteAutoExposure(xiiView& view, const AutoExposureData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("AutoExposure");
  cmd.SetPipelineState(view.m_ViewPassResources.m_TemporalPasses.m_pAutoExposurePipeline);
  BIND_SRV_BUF("g_Histogram", data.m_hHistogram);
  BIND_UAV_TEX("g_Exposure", data.m_hExposure);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({1u, 1u, 1u});
  cmd.EndDebugGroup();
}

static void SetupTAA(xiiView& view, TAAData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TemporalPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_HDRSceneColor);
  READ_TEX(data.m_hVelocity, xiiRGBlackboardKeys::k_VelocityBuffer);

  if (!tp.m_pTAAHistoryBuffer)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType     = xiiGALTextureType::Texture2D;
    desc.m_Format          = xiiGALTextureFormat::RGBA16Float;
    desc.m_uiWidth         = data.m_W;
    desc.m_uiHeight        = data.m_H;
    desc.m_uiMipLevels     = 1u;
    desc.m_BindFlags       = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Usage           = xiiGALResourceUsage::Default;
    tp.m_pTAAHistoryBuffer = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
  }
  data.m_hHistory = builder.ImportTexture("TAAHistory", tp.m_pTAAHistoryBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHistory = builder.ReadTexture(data.m_hHistory, xiiGALResourceStateFlags::ShaderResource);
  WRITE_TEX_HDR(data.m_hTAAOut, xiiRGBlackboardKeys::k_TAAResolvedColor, data.m_W, data.m_H);
  xiiView::EnsureComputePipeline(tp.m_pTAAPipeline, "Shaders/Pipeline/TAA.xiiShader");
}
static void ExecuteTAA(xiiView& view, const TAAData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("TAA");
  cmd.SetPipelineState(view.m_ViewPassResources.m_TemporalPasses.m_pTAAPipeline);
  BIND_SRV_TEX("g_HDRCurrent", data.m_hHDRIn);
  BIND_SRV_TEX("g_Velocity", data.m_hVelocity);
  BIND_SRV_TEX("g_History", data.m_hHistory);
  BIND_UAV_TEX("g_TAAOut", data.m_hTAAOut);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupUpscale(xiiView& view, UpscaleData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& tp = view.m_ViewPassResources.m_TemporalPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hTAAIn, xiiRGBlackboardKeys::k_TAAResolvedColor);
  // Upscaled target at native viewport resolution.
  const xiiUInt32                  uiNW = static_cast<xiiUInt32>(view.GetViewport().width);
  const xiiUInt32                  uiNH = static_cast<xiiUInt32>(view.GetViewport().height);
  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::RGBA16Float;
  desc.m_uiWidth     = uiNW;
  desc.m_uiHeight    = uiNH;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hUpscaled   = builder.WriteTexture(xiiRGBlackboardKeys::k_UpscaledColor, desc, xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(tp.m_pUpscalePipeline, "Shaders/Pipeline/CASUpscale.xiiShader");
}
static void ExecuteUpscale(xiiView& view, const UpscaleData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("Upscale");
  cmd.SetPipelineState(view.m_ViewPassResources.m_TemporalPasses.m_pUpscalePipeline);
  BIND_SRV_TEX("g_TAAIn", data.m_hTAAIn);
  BIND_UAV_TEX("g_Upscaled", data.m_hUpscaled);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(static_cast<xiiUInt32>(view.GetViewport().width), static_cast<xiiUInt32>(view.GetViewport().height));
  cmd.EndDebugGroup();
}

void xiiView::BuildStage10_Temporal(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView*                        self = this;
  const xiiRenderGraphBlackboard& bb   = blackboard;

  graph.AddPass<LumHistogramData>("LuminanceHistogram", xiiGALCommandQueueFlags::Compute, [self, &bb](LumHistogramData& d, xiiRGBuilder& b) { SetupLumHistogram(*self, d, b, bb); }, [self](const LumHistogramData& d, xiiRGPassContext& c) { ExecuteLumHistogram(*self, d, c); });

  graph.AddPass<AutoExposureData>("AutoExposure", xiiGALCommandQueueFlags::Compute, [self, &bb](AutoExposureData& d, xiiRGBuilder& b) { SetupAutoExposure(*self, d, b, bb); }, [self](const AutoExposureData& d, xiiRGPassContext& c) { ExecuteAutoExposure(*self, d, c); });

  graph.AddPass<TAAData>("TAA", xiiGALCommandQueueFlags::Compute, [self, &bb](TAAData& d, xiiRGBuilder& b) { SetupTAA(*self, d, b, bb); }, [self](const TAAData& d, xiiRGPassContext& c) { ExecuteTAA(*self, d, c); });

  graph.AddPass<UpscaleData>("Upscale", xiiGALCommandQueueFlags::Compute, [self, &bb](UpscaleData& d, xiiRGBuilder& b) { SetupUpscale(*self, d, b, bb); }, [self](const UpscaleData& d, xiiRGPassContext& c) { ExecuteUpscale(*self, d, c); });
}

// ═════════════════════════════════════════════════════════════════════════════
// STAGE 11 - Post-processing
// ═════════════════════════════════════════════════════════════════════════════
namespace
{
  struct BloomData
  {
    xiiRGTextureHandle m_hHDRIn, m_hExposure, m_hBloom;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct ColorGradeData
  {
    xiiRGTextureHandle m_hHDRIn, m_hExposure, m_hBloom, m_hGraded;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct ToneMappingData
  {
    xiiRGTextureHandle m_hGraded, m_hLDROut;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
  };
  struct PostFxData
  {
    xiiRGTextureHandle m_hIn, m_hOut;
    xiiUInt32          m_W = 1920u, m_H = 1080u;
    const char*        m_szShader = "";
    const char*        m_szLabel  = "";
  };
} // namespace

static void SetupBloom(xiiView& view, BloomData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& pp = view.m_ViewPassResources.m_PostProcessPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_UpscaledColor);
  WRITE_TEX_HDR(data.m_hBloom, xiiRGBlackboardKeys::k_BloomTexture, data.m_W, data.m_H);
  xiiView::EnsureComputePipeline(pp.m_pBloomPipeline, "Shaders/Pipeline/BloomChain.xiiShader");
}
static void ExecuteBloom(xiiView& view, const BloomData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("Bloom");
  cmd.SetPipelineState(view.m_ViewPassResources.m_PostProcessPasses.m_pBloomPipeline);
  BIND_SRV_TEX("g_HDRIn", data.m_hHDRIn);
  BIND_UAV_TEX("g_BloomOut", data.m_hBloom);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupColorGrade(xiiView& view, ColorGradeData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& pp = view.m_ViewPassResources.m_PostProcessPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hHDRIn, xiiRGBlackboardKeys::k_UpscaledColor);
  READ_TEX(data.m_hBloom, xiiRGBlackboardKeys::k_BloomTexture);
  WRITE_TEX_HDR(data.m_hGraded, xiiRGBlackboardKeys::k_GradedColor, data.m_W, data.m_H);
  xiiView::EnsureComputePipeline(pp.m_pColorGradingPipeline, "Shaders/Pipeline/ColorGrading.xiiShader");
}
static void ExecuteColorGrade(xiiView& view, const ColorGradeData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("ColorGrading");
  cmd.SetPipelineState(view.m_ViewPassResources.m_PostProcessPasses.m_pColorGradingPipeline);
  BIND_SRV_TEX("g_HDRIn", data.m_hHDRIn);
  BIND_SRV_TEX("g_Bloom", data.m_hBloom);
  BIND_UAV_TEX("g_Graded", data.m_hGraded);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

static void SetupToneMapping(xiiView& view, ToneMappingData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& pp = view.m_ViewPassResources.m_PostProcessPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_W);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_H);
  READ_TEX(data.m_hGraded, xiiRGBlackboardKeys::k_GradedColor);
  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::RGBA8Unorm;
  desc.m_uiWidth     = data.m_W;
  desc.m_uiHeight    = data.m_H;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hLDROut     = builder.WriteTexture(xiiRGBlackboardKeys::k_LDRSceneColor, desc, xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(pp.m_pToneMappingPipeline, "Shaders/Pipeline/ToneMapping.xiiShader");
}
static void ExecuteToneMapping(xiiView& view, const ToneMappingData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  cmd.BeginDebugGroup("ToneMapping");
  cmd.SetPipelineState(view.m_ViewPassResources.m_PostProcessPasses.m_pToneMappingPipeline);
  BIND_SRV_TEX("g_HDRGraded", data.m_hGraded);
  BIND_UAV_TEX("g_LDROut", data.m_hLDROut);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  DISPATCH(data.m_W, data.m_H);
  cmd.EndDebugGroup();
}

void xiiView::BuildStage11_PostProcess(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView*                        self = this;
  const xiiRenderGraphBlackboard& bb   = blackboard;

  graph.AddPass<BloomData>("Bloom", xiiGALCommandQueueFlags::Compute, [self, &bb](BloomData& d, xiiRGBuilder& b) { SetupBloom(*self, d, b, bb); }, [self](const BloomData& d, xiiRGPassContext& c) { ExecuteBloom(*self, d, c); });

  graph.AddPass<ColorGradeData>("ColorGrading", xiiGALCommandQueueFlags::Compute, [self, &bb](ColorGradeData& d, xiiRGBuilder& b) { SetupColorGrade(*self, d, b, bb); }, [self](const ColorGradeData& d, xiiRGPassContext& c) { ExecuteColorGrade(*self, d, c); });

  graph.AddPass<ToneMappingData>("ToneMapping", xiiGALCommandQueueFlags::Compute, [self, &bb](ToneMappingData& d, xiiRGBuilder& b) { SetupToneMapping(*self, d, b, bb); }, [self](const ToneMappingData& d, xiiRGPassContext& c) { ExecuteToneMapping(*self, d, c); });
}

// ═════════════════════════════════════════════════════════════════════════════
// STAGE 12 - Final output
// ═════════════════════════════════════════════════════════════════════════════
namespace
{
  struct FinalBlitData
  {
    xiiRGTextureHandle m_hLDRIn, m_hBackbuffer;
    xiiUInt32          m_uiW = 1920u, m_uiH = 1080u;
  };
} // namespace

static void SetupFinalBlit(xiiView& view, FinalBlitData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& op = view.m_ViewPassResources.m_OutputPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiH);
  READ_TEX(data.m_hLDRIn, xiiRGBlackboardKeys::k_LDRSceneColor);

  // Import the swapchain backbuffer as the write target.
  if (xiiGALSwapChain* pSC = view.GetSwapChain())
  {
    xiiSharedPtr<xiiGALTexture> pBB = pSC->GetCurrentBackBuffer();
    data.m_hBackbuffer              = builder.ImportTexture("Backbuffer", pBB, xiiGALResourceStateFlags::RenderTarget);
    data.m_hBackbuffer              = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
  }

  xiiView::EnsureComputePipeline(op.m_pFinalBlitPipeline, "Shaders/Pipeline/FinalBlit.xiiShader");
  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

static void ExecuteFinalBlit(xiiView& view, const FinalBlitData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              op  = view.m_ViewPassResources.m_OutputPasses;

  cmd.BeginDebugGroup("BackbufferPresent");
  if (data.m_hLDRIn.IsValid() && data.m_hBackbuffer.IsValid() && op.m_pFinalBlitPipeline)
  {
    const float fClear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    cmd.ClearRenderTarget(ctx.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fClear, xiiGALStateTransitionMode::Transition);
    cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiW), static_cast<float>(data.m_uiH), 0.0f, 1.0f}}, data.m_uiW, data.m_uiH);
    cmd.SetPipelineState(op.m_pFinalBlitPipeline);
    cmd.ResolveAndSetShaderResourceView("g_LDRIn", ctx.GetTexture(data.m_hLDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    // Fullscreen triangle draw (no vertex buffer, vertex shader generates clip-space positions).
    cmd.Draw({3u, 1u, 0u, 0u});
  }
  cmd.EndDebugGroup();
}

void xiiView::BuildStage12_Output(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView*                        self = this;
  const xiiRenderGraphBlackboard& bb   = blackboard;

  graph.AddPass<FinalBlitData>(
    "BackbufferPresent", xiiGALCommandQueueFlags::Graphics,
    [self, &bb](FinalBlitData& d, xiiRGBuilder& b) { SetupFinalBlit(*self, d, b, bb); },
    [self](const FinalBlitData& d, xiiRGPassContext& c) { ExecuteFinalBlit(*self, d, c); });
}
