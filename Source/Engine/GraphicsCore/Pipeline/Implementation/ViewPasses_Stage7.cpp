// XII Engine - ViewPasses_Stage7.cpp
// Stage 7: Forward passes (all Graphics).
// ForwardOpaque → ForwardMasked → Hair → Water → Subsurface Scattering → Eye

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

// 
// Forward opaque (graphics - produces HDRSceneColor by compositing lighting)
// 
namespace
{
  struct ForwardOpaqueData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hDirectLighting, m_hIndirectLighting;
    xiiRGTextureHandle m_hRTFinalGI, m_hRTFinalRefl, m_hSSR;
    xiiRGTextureHandle m_hVolumetricScattering, m_hAO;
    xiiRGBufferHandle  m_hLightGrid, m_hLightIndex;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiRGTextureHandle m_hHDRSceneColor;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupForwardOpaque(xiiView& view, ForwardOpaqueData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  auto readTex = [&](xiiRGTextureHandle& h, const char* k)
  { xiiRGTextureHandle t; bb.TryGetValue(xiiMakeHashedString(k), t); if (t.IsValid()) h = builder.ReadTexture(t, xiiGALResourceStateFlags::ShaderResource); };
  auto readBuf = [&](xiiRGBufferHandle& h, const char* k)
  { xiiRGBufferHandle b2; bb.TryGetValue(xiiMakeHashedString(k), b2); if (b2.IsValid()) h = builder.ReadBuffer(b2, xiiGALResourceStateFlags::ShaderResource); };

  readTex(data.m_hSceneDepth,        xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hDirectLighting,    xiiRGBlackboardKeys::k_DirectLightingBuffer);
  readTex(data.m_hIndirectLighting,  xiiRGBlackboardKeys::k_IndirectLightingBuffer);
  readTex(data.m_hRTFinalGI,        xiiRGBlackboardKeys::k_RTFinalGI);
  readTex(data.m_hRTFinalRefl,      xiiRGBlackboardKeys::k_RTFinalReflections);
  readTex(data.m_hSSR,              xiiRGBlackboardKeys::k_SSRTexture);
  readTex(data.m_hVolumetricScattering, xiiRGBlackboardKeys::k_VolumetricScattering);
  readTex(data.m_hAO,               xiiRGBlackboardKeys::k_StableAOTexture);
  readBuf(data.m_hLightGrid,        xiiRGBlackboardKeys::k_LightGridBuffer);
  readBuf(data.m_hLightIndex,       xiiRGBlackboardKeys::k_LightIndexBuffer);
  {
    xiiRGBufferHandle h; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), h);
    if (h.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(h, xiiGALResourceStateFlags::IndirectArgument);
  }

  // HDRSceneColor - the main composited output, also reads existing depth-stencil.
  xiiGALTextureCreationDescription hdrDesc;
  hdrDesc.m_TextureType = xiiGALTextureType::Texture2D;
  hdrDesc.m_Format      = xiiGALTextureFormat::RGBA16Float;
  hdrDesc.m_uiWidth     = data.m_uiRenderW;
  hdrDesc.m_uiHeight    = data.m_uiRenderH;
  hdrDesc.m_uiMipLevels = 1u;
  hdrDesc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  hdrDesc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, hdrDesc, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

static void ExecuteForwardOpaque(xiiView& view, const ForwardOpaqueData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;

  cmd.BeginDebugGroup("ForwardOpaque");
  const float fClear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fClear, xiiGALStateTransitionMode::Transition);
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);

  if (fp.m_pForwardOpaquePipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(fp.m_pForwardOpaquePipeline);
    auto bindSRV = [&](const char* s, xiiRGTextureHandle h)
    { if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel); };
    bindSRV("g_DirectLight",   data.m_hDirectLighting);
    bindSRV("g_IndirectLight", data.m_hIndirectLighting);
    bindSRV("g_RTRefl",        data.m_hRTFinalRefl);
    bindSRV("g_RTGI",          data.m_hRTFinalGI);
    bindSRV("g_SSR",           data.m_hSSR);
    bindSRV("g_Volumetric",    data.m_hVolumetricScattering);
    bindSRV("g_AO",            data.m_hAO);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }
  cmd.EndDebugGroup();
}

// 
// Forward masked (alpha-test)
// 
namespace
{
  struct ForwardMaskedData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupForwardMasked(xiiView& view, ForwardMaskedData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hHDR; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);
  if (hHDR.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(hHDR, xiiGALResourceStateFlags::RenderTarget);
  xiiRGTextureHandle hDepth; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  if (hDepth.IsValid()) data.m_hSceneDepth = builder.WriteTexture(hDepth, xiiGALResourceStateFlags::DepthWrite);
  xiiRGBufferHandle hDraw; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDraw);
  if (hDraw.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(hDraw, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

static void ExecuteForwardMasked(xiiView& view, const ForwardMaskedData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;
  cmd.BeginDebugGroup("ForwardMasked");
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);
  if (fp.m_pForwardMaskedPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(fp.m_pForwardMaskedPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }
  cmd.EndDebugGroup();
}

// 
// Hair rendering (strand-based, Marschner BSDF)
// 
namespace
{
  struct HairData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupHair(xiiView& view, HairData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle h; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), h);
  if (h.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(h, xiiGALResourceStateFlags::RenderTarget);
  xiiRGTextureHandle hD; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hD);
  if (hD.IsValid()) data.m_hSceneDepth = builder.WriteTexture(hD, xiiGALResourceStateFlags::DepthWrite);
  xiiRGBufferHandle hC; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hC);
  if (hC.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(hC, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

static void ExecuteHair(xiiView& view, const HairData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;
  cmd.BeginDebugGroup("HairRendering");
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);
  if (fp.m_pHairPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(fp.m_pHairPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }
  cmd.EndDebugGroup();
}

// 
// Water rendering (planar reflection + refraction composite)
// 
namespace
{
  struct WaterData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth, m_hPlanarRefl;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupWater(xiiView& view, WaterData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle h; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), h);
  if (h.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(h, xiiGALResourceStateFlags::RenderTarget);
  xiiRGTextureHandle hD; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hD);
  if (hD.IsValid()) data.m_hSceneDepth = builder.WriteTexture(hD, xiiGALResourceStateFlags::DepthWrite);
  xiiRGTextureHandle hPR; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_PlanarReflectionMap), hPR);
  if (hPR.IsValid()) data.m_hPlanarRefl = builder.ReadTexture(hPR, xiiGALResourceStateFlags::ShaderResource);
  xiiRGBufferHandle hC; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hC);
  if (hC.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(hC, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

static void ExecuteWater(xiiView& view, const WaterData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;
  cmd.BeginDebugGroup("WaterRendering");
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);
  if (fp.m_pWaterPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(fp.m_pWaterPipeline);
    if (data.m_hPlanarRefl.IsValid())
      cmd.ResolveAndSetShaderResourceView("g_PlanarRefl", ctx.GetTexture(data.m_hPlanarRefl)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }
  cmd.EndDebugGroup();
}

// 
// Screen-space subsurface scattering blur (compute)
// 
namespace
{
  struct SSSData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth, m_hGBufMaterial;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupSSS(xiiView& view, SSSData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle h; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), h);
  if (h.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(h, xiiGALResourceStateFlags::UnorderedAccess);
  xiiRGTextureHandle hD; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hD);
  if (hD.IsValid()) data.m_hSceneDepth = builder.ReadTexture(hD, xiiGALResourceStateFlags::ShaderResource);
  xiiRGTextureHandle hM; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferMaterial), hM);
  if (hM.IsValid()) data.m_hGBufMaterial = builder.ReadTexture(hM, xiiGALResourceStateFlags::ShaderResource);
}

static void ExecuteSSS(xiiView& view, const SSSData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;
  cmd.BeginDebugGroup("SubsurfaceScattering");
  if (fp.m_pSSSComputePipeline)
  {
    cmd.SetPipelineState(fp.m_pSSSComputePipeline);
    if (data.m_hSceneDepth.IsValid())   cmd.ResolveAndSetShaderResourceView("g_SceneDepth",  ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),     xiiGALShaderType::Compute);
    if (data.m_hGBufMaterial.IsValid()) cmd.ResolveAndSetShaderResourceView("g_GBufMaterial",ctx.GetTexture(data.m_hGBufMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource),  xiiGALShaderType::Compute);
    if (data.m_hHDRSceneColor.IsValid()) cmd.ResolveAndSetUnorderedAccessView("g_HDRInOut",  ctx.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  }
  cmd.EndDebugGroup();
}

// 
// Eye shader (cornea refraction + limbal ring)
// 
namespace
{
  struct EyeData
  {
    xiiRGTextureHandle m_hHDRSceneColor, m_hSceneDepth;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
}

static void SetupEye(xiiView& view, EyeData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& fp = view.m_ViewPassResources.m_ForwardPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle h; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), h);
  if (h.IsValid()) data.m_hHDRSceneColor = builder.WriteTexture(h, xiiGALResourceStateFlags::RenderTarget);
  xiiRGTextureHandle hD; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hD);
  if (hD.IsValid()) data.m_hSceneDepth = builder.WriteTexture(hD, xiiGALResourceStateFlags::DepthWrite);
  xiiRGBufferHandle hC; bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hC);
  if (hC.IsValid()) data.m_hDrawCommands = builder.ReadBuffer(hC, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

static void ExecuteEye(xiiView& view, const EyeData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto& fp               = view.m_ViewPassResources.m_ForwardPasses;
  cmd.BeginDebugGroup("EyeShader");
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);
  if (fp.m_pEyePipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(fp.m_pEyePipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }
  cmd.EndDebugGroup();
}

// 
// BuildStage7_Forward - entry point
// 

void xiiView::BuildStage7_Forward(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  graph.AddPass<ForwardOpaqueData>(
    "ForwardOpaque", xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](ForwardOpaqueData& d, xiiRGBuilder& b) { SetupForwardOpaque(*self, d, b, blackboard); },
    [self](const ForwardOpaqueData& d, xiiRGPassContext& c) { ExecuteForwardOpaque(*self, d, c); });

  graph.AddPass<ForwardMaskedData>(
    "ForwardMasked", xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](ForwardMaskedData& d, xiiRGBuilder& b) { SetupForwardMasked(*self, d, b, blackboard); },
    [self](const ForwardMaskedData& d, xiiRGPassContext& c) { ExecuteForwardMasked(*self, d, c); });

  graph.AddPass<HairData>(
    "HairRendering", xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](HairData& d, xiiRGBuilder& b) { SetupHair(*self, d, b, blackboard); },
    [self](const HairData& d, xiiRGPassContext& c) { ExecuteHair(*self, d, c); });

  graph.AddPass<WaterData>(
    "WaterRendering", xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](WaterData& d, xiiRGBuilder& b) { SetupWater(*self, d, b, blackboard); },
    [self](const WaterData& d, xiiRGPassContext& c) { ExecuteWater(*self, d, c); });

  graph.AddPass<SSSData>(
    "SubsurfaceScattering", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](SSSData& d, xiiRGBuilder& b) { SetupSSS(*self, d, b, blackboard); },
    [self](const SSSData& d, xiiRGPassContext& c) { ExecuteSSS(*self, d, c); });

  graph.AddPass<EyeData>(
    "EyeShader", xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](EyeData& d, xiiRGBuilder& b) { SetupEye(*self, d, b, blackboard); },
    [self](const EyeData& d, xiiRGPassContext& c) { ExecuteEye(*self, d, c); });
}
