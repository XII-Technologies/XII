// XII Engine - ViewPasses_Stage6.cpp
// Stage 6: Main lighting (all compute / RT).
// Deferred direct -> indirect lighting -> RT GI -> RT reflections -> SSR ->
// Volumetric integrate -> Volumetric temporal reprojection -> Atmosphere composite.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

//
// Helper: declare a full-resolution RGBA16F UAV output texture.
//
static xiiRGTextureHandle DeclareHDROutput(xiiRGBuilder& builder, const char* szKey, xiiUInt32 uiW, xiiUInt32 uiH)
{
  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::RGBA16Float;
  desc.m_uiWidth     = uiW;
  desc.m_uiHeight    = uiH;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  return builder.WriteTexture(szKey, desc, xiiGALResourceStateFlags::UnorderedAccess);
}

//
// Deferred direct lighting
//
namespace
{
  struct DirectLightData
  {
    xiiRGTextureHandle m_hAlbedo, m_hNormal, m_hMaterial, m_hSceneDepth;
    xiiRGTextureHandle m_hAO, m_hRTShadow, m_hContactShadow, m_hDirShadowAtlas;
    xiiRGBufferHandle  m_hLightGrid, m_hLightIndex;
    xiiRGTextureHandle m_hDirectOut;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupDirectLighting(xiiView& view, DirectLightData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  auto readTex = [&](xiiRGTextureHandle& hOut, const char* szKey) {
    xiiRGTextureHandle h;
    bb.TryGetValue(xiiMakeHashedString(szKey), h);
    if (h.IsValid()) hOut = builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource);
  };
  auto readBuf = [&](xiiRGBufferHandle& hOut, const char* szKey) {
    xiiRGBufferHandle h;
    bb.TryGetValue(xiiMakeHashedString(szKey), h);
    if (h.IsValid()) hOut = builder.ReadBuffer(h, xiiGALResourceStateFlags::ShaderResource);
  };

  readTex(data.m_hAlbedo, xiiRGBlackboardKeys::k_GBufferAlbedo);
  readTex(data.m_hNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  readTex(data.m_hMaterial, xiiRGBlackboardKeys::k_GBufferMaterial);
  readTex(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hAO, xiiRGBlackboardKeys::k_StableAOTexture);
  readTex(data.m_hRTShadow, xiiRGBlackboardKeys::k_RTFinalShadowMask);
  readTex(data.m_hContactShadow, xiiRGBlackboardKeys::k_ContactShadowTerm);
  readTex(data.m_hDirShadowAtlas, xiiRGBlackboardKeys::k_DirectionalShadowAtlas);
  readBuf(data.m_hLightGrid, xiiRGBlackboardKeys::k_LightGridBuffer);
  readBuf(data.m_hLightIndex, xiiRGBlackboardKeys::k_LightIndexBuffer);

  data.m_hDirectOut = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_DirectLightingBuffer, data.m_uiRenderW, data.m_uiRenderH);

  xiiView::EnsureComputePipeline(lp.m_pDirectLightingPipeline, "Shaders/Pipeline/DirectLighting.xiiShader");
}

static void ExecuteDirectLighting(xiiView& view, const DirectLightData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPasses;

  cmd.BeginDebugGroup("DeferredDirectLighting");
  cmd.SetPipelineState(lp.m_pDirectLightingPipeline);

  auto bindSRV = [&](const char* szSlot, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(szSlot, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  auto bindBufSRV = [&](const char* szSlot, xiiRGBufferHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceBufferView(szSlot, ctx.GetBuffer(h)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  };

  bindSRV("g_GBufAlbedo", data.m_hAlbedo);
  bindSRV("g_GBufNormal", data.m_hNormal);
  bindSRV("g_GBufMaterial", data.m_hMaterial);
  bindSRV("g_SceneDepth", data.m_hSceneDepth);
  bindSRV("g_AOTerm", data.m_hAO);
  bindSRV("g_RTShadow", data.m_hRTShadow);
  bindSRV("g_ContactShadow", data.m_hContactShadow);
  bindSRV("g_ShadowAtlas", data.m_hDirShadowAtlas);
  bindBufSRV("g_LightGrid", data.m_hLightGrid);
  bindBufSRV("g_LightIndex", data.m_hLightIndex);
  cmd.ResolveAndSetUnorderedAccessView("g_DirectOut", ctx.GetTexture(data.m_hDirectOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Deferred indirect lighting
//
namespace
{
  struct IndirectLightData
  {
    xiiRGTextureHandle m_hAlbedo, m_hNormal, m_hMaterial, m_hSceneDepth;
    xiiRGTextureHandle m_hAO, m_hBRDFLut, m_hDDGI, m_hSkyRadiance;
    xiiRGTextureHandle m_hIndirectOut;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupIndirectLighting(xiiView& view, IndirectLightData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  auto readTex = [&](xiiRGTextureHandle& hOut, const char* szKey) {
    xiiRGTextureHandle h;
    bb.TryGetValue(xiiMakeHashedString(szKey), h);
    if (h.IsValid()) hOut = builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource);
  };

  readTex(data.m_hAlbedo, xiiRGBlackboardKeys::k_GBufferAlbedo);
  readTex(data.m_hNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  readTex(data.m_hMaterial, xiiRGBlackboardKeys::k_GBufferMaterial);
  readTex(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hAO, xiiRGBlackboardKeys::k_StableAOTexture);
  readTex(data.m_hBRDFLut, xiiRGBlackboardKeys::k_BRDFLut);
  readTex(data.m_hDDGI, xiiRGBlackboardKeys::k_DDGIIrradiance);
  readTex(data.m_hSkyRadiance, xiiRGBlackboardKeys::k_SkyRadiance);

  data.m_hIndirectOut = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_IndirectLightingBuffer, data.m_uiRenderW, data.m_uiRenderH);
  xiiView::EnsureComputePipeline(lp.m_pIndirectLightingPipeline, "Shaders/Pipeline/IndirectLighting.xiiShader");
}

static void ExecuteIndirectLighting(xiiView& view, const IndirectLightData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPasses;

  cmd.BeginDebugGroup("DeferredIndirectLighting");
  cmd.SetPipelineState(lp.m_pIndirectLightingPipeline);
  auto bindSRV = [&](const char* s, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  bindSRV("g_GBufAlbedo", data.m_hAlbedo);
  bindSRV("g_GBufNormal", data.m_hNormal);
  bindSRV("g_GBufMaterial", data.m_hMaterial);
  bindSRV("g_SceneDepth", data.m_hSceneDepth);
  bindSRV("g_AOTerm", data.m_hAO);
  bindSRV("g_BRDFLut", data.m_hBRDFLut);
  bindSRV("g_DDGIIr", data.m_hDDGI);
  bindSRV("g_SkyRadiance", data.m_hSkyRadiance);
  cmd.ResolveAndSetUnorderedAccessView("g_IndirectOut", ctx.GetTexture(data.m_hIndirectOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// RT GI final gather + denoise
//
namespace
{
  struct RTGIData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hGBufNormal, m_hIndirectIn;
    xiiRGTextureHandle m_hRTRawGI, m_hRTFinalGI;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupRTGI(xiiView& view, RTGIData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);
  auto readTex = [&](xiiRGTextureHandle& h, const char* k) {
    xiiRGTextureHandle t;
    bb.TryGetValue(xiiMakeHashedString(k), t);
    if (t.IsValid()) h = builder.ReadTexture(t, xiiGALResourceStateFlags::ShaderResource);
  };
  readTex(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  readTex(data.m_hIndirectIn, xiiRGBlackboardKeys::k_IndirectLightingBuffer);
  data.m_hRTRawGI   = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_RTRawGI, data.m_uiRenderW, data.m_uiRenderH);
  data.m_hRTFinalGI = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_RTFinalGI, data.m_uiRenderW, data.m_uiRenderH);
  xiiView::EnsureComputePipeline(lp.m_pRTGIPipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

static void ExecuteRTGI(xiiView& view, const RTGIData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd     = ctx.GetCommandList();
  auto&              lp      = view.m_ViewPassResources.m_LightingPasses;
  auto               bindSRV = [&](const char* s, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  cmd.BeginDebugGroup("RTGIFinalGather");
  cmd.SetPipelineState(lp.m_pRTGIPipeline);
  bindSRV("g_SceneDepth", data.m_hSceneDepth);
  bindSRV("g_GBufNormal", data.m_hGBufNormal);
  bindSRV("g_IndirectIn", data.m_hIndirectIn);
  cmd.ResolveAndSetUnorderedAccessView("g_RTGIRaw", ctx.GetTexture(data.m_hRTRawGI)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_RTGIFinal", ctx.GetTexture(data.m_hRTFinalGI)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// RT reflections + denoise
//
namespace
{
  struct RTReflData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hGBufNormal, m_hGBufMaterial, m_hBRDFLut;
    xiiRGTextureHandle m_hRTRawRefl, m_hRTFinalRefl;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupRTReflections(xiiView& view, RTReflData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);
  auto readTex = [&](xiiRGTextureHandle& h, const char* k) {
    xiiRGTextureHandle t;
    bb.TryGetValue(xiiMakeHashedString(k), t);
    if (t.IsValid()) h = builder.ReadTexture(t, xiiGALResourceStateFlags::ShaderResource);
  };
  readTex(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  readTex(data.m_hGBufMaterial, xiiRGBlackboardKeys::k_GBufferMaterial);
  readTex(data.m_hBRDFLut, xiiRGBlackboardKeys::k_BRDFLut);
  data.m_hRTRawRefl   = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_RTRawReflections, data.m_uiRenderW, data.m_uiRenderH);
  data.m_hRTFinalRefl = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_RTFinalReflections, data.m_uiRenderW, data.m_uiRenderH);
  xiiView::EnsureComputePipeline(lp.m_pRTReflectionPipeline, "Shaders/Pipeline/RTReflection.xiiShader");
}

static void ExecuteRTReflections(xiiView& view, const RTReflData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd     = ctx.GetCommandList();
  auto&              lp      = view.m_ViewPassResources.m_LightingPasses;
  auto               bindSRV = [&](const char* s, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  cmd.BeginDebugGroup("RTReflections");
  cmd.SetPipelineState(lp.m_pRTReflectionPipeline);
  bindSRV("g_SceneDepth", data.m_hSceneDepth);
  bindSRV("g_GBufNormal", data.m_hGBufNormal);
  bindSRV("g_GBufMaterial", data.m_hGBufMaterial);
  bindSRV("g_BRDFLut", data.m_hBRDFLut);
  cmd.ResolveAndSetUnorderedAccessView("g_RTReflRaw", ctx.GetTexture(data.m_hRTRawRefl)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_RTReflFinal", ctx.GetTexture(data.m_hRTFinalRefl)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Screen-space reflections
//
namespace
{
  struct SSRData
  {
    xiiRGTextureHandle m_hSceneDepth, m_hGBufNormal, m_hGBufMaterial, m_hHDRIn, m_hSSR;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupSSR(xiiView& view, SSRData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);
  auto readTex = [&](xiiRGTextureHandle& h, const char* k) {
    xiiRGTextureHandle t;
    bb.TryGetValue(xiiMakeHashedString(k), t);
    if (t.IsValid()) h = builder.ReadTexture(t, xiiGALResourceStateFlags::ShaderResource);
  };
  readTex(data.m_hSceneDepth, xiiRGBlackboardKeys::k_SceneDepthTexture);
  readTex(data.m_hGBufNormal, xiiRGBlackboardKeys::k_GBufferNormal);
  readTex(data.m_hGBufMaterial, xiiRGBlackboardKeys::k_GBufferMaterial);
  readTex(data.m_hHDRIn, xiiRGBlackboardKeys::k_HDRSceneColor); // reads current scene color for reflections
  data.m_hSSR = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_SSRTexture, data.m_uiRenderW, data.m_uiRenderH);
  xiiView::EnsureComputePipeline(lp.m_pSSRPipeline, "Shaders/Pipeline/SSR.xiiShader");
}

static void ExecuteSSR(xiiView& view, const SSRData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd     = ctx.GetCommandList();
  auto&              lp      = view.m_ViewPassResources.m_LightingPasses;
  auto               bindSRV = [&](const char* s, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  cmd.BeginDebugGroup("SSR");
  cmd.SetPipelineState(lp.m_pSSRPipeline);
  bindSRV("g_SceneDepth", data.m_hSceneDepth);
  bindSRV("g_GBufNormal", data.m_hGBufNormal);
  bindSRV("g_GBufMaterial", data.m_hGBufMaterial);
  bindSRV("g_HDRScene", data.m_hHDRIn);
  cmd.ResolveAndSetUnorderedAccessView("g_SSROut", ctx.GetTexture(data.m_hSSR)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Volumetric fog integration
//
namespace
{
  struct VolumetricIntegrateData
  {
    xiiRGTextureHandle m_hFroxelScattering, m_hLightGrid, m_hVolumetricOut;
    xiiRGBufferHandle  m_hLightIndex;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupVolumetricIntegrate(xiiView& view, VolumetricIntegrateData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hScat;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), hScat);
  if (hScat.IsValid()) data.m_hFroxelScattering = builder.ReadTexture(hScat, xiiGALResourceStateFlags::ShaderResource);
  xiiRGBufferHandle hGrid;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightGridBuffer), hGrid);
  if (hGrid.IsValid()) data.m_hLightIndex = builder.ReadBuffer(hGrid, xiiGALResourceStateFlags::ShaderResource);

  data.m_hVolumetricOut = DeclareHDROutput(builder, xiiRGBlackboardKeys::k_VolumetricScattering, data.m_uiRenderW, data.m_uiRenderH);
  xiiView::EnsureComputePipeline(lp.m_pVolumetricIntegratePipeline, "Shaders/Pipeline/VolumetricLightIntegration.xiiShader");
}

static void ExecuteVolumetricIntegrate(xiiView& view, const VolumetricIntegrateData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPasses;
  cmd.BeginDebugGroup("VolumetricFogIntegrate");
  cmd.SetPipelineState(lp.m_pVolumetricIntegratePipeline);
  if (data.m_hFroxelScattering.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_FroxelScattering", ctx.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hLightIndex.IsValid())
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", ctx.GetBuffer(data.m_hLightIndex)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_VolumetricOut", ctx.GetTexture(data.m_hVolumetricOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Volumetric fog temporal reprojection
//
namespace
{
  struct VolumetricTemporalData
  {
    xiiRGTextureHandle m_hCurrentFroxel, m_hVolumetricScattering;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupVolumetricTemporal(xiiView& view, VolumetricTemporalData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  // History froxel from previous frame.
  if (!lp.m_pFroxelHistoryBuffer)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType        = xiiGALTextureType::Texture3D;
    desc.m_Format             = xiiGALTextureFormat::RGBA16Float;
    desc.m_uiWidth            = 128u;
    desc.m_uiHeight           = 72u;
    desc.m_uiDepth            = 64u;
    desc.m_uiMipLevels        = 1u;
    desc.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Usage              = xiiGALResourceUsage::Default;
    lp.m_pFroxelHistoryBuffer = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
  }
  // History is read this frame, then we write back the blended result.
  data.m_hCurrentFroxel = builder.ImportTexture("FroxelHistory", lp.m_pFroxelHistoryBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCurrentFroxel = builder.ReadTexture(data.m_hCurrentFroxel, xiiGALResourceStateFlags::ShaderResource);

  xiiRGTextureHandle hScat;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VolumetricScattering), hScat);
  if (hScat.IsValid()) data.m_hVolumetricScattering = builder.WriteTexture(hScat, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pVolumetricTemporalPipeline, "Shaders/Pipeline/VolumetricFogTemporalRep.xiiShader");
}

static void ExecuteVolumetricTemporal(xiiView& view, const VolumetricTemporalData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPasses;
  cmd.BeginDebugGroup("VolumetricFogTemporalRep");
  cmd.SetPipelineState(lp.m_pVolumetricTemporalPipeline);
  if (data.m_hCurrentFroxel.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_FroxelHistory", ctx.GetTexture(data.m_hCurrentFroxel)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hVolumetricScattering.IsValid())
    cmd.ResolveAndSetUnorderedAccessView("g_FroxelBlended", ctx.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({16u, 9u, 8u});
  cmd.EndDebugGroup();
}

//
// Atmosphere composite -> sky radiance accumulation
//
namespace
{
  struct AtmCompositeData
  {
    xiiRGTextureHandle m_hTransmittanceLUT, m_hMultiScatterLUT, m_hVolumetricScattering;
    xiiRGTextureHandle m_hSkyRadiance;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupAtmComposite(xiiView& view, AtmCompositeData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPasses;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);
  auto readTex = [&](xiiRGTextureHandle& h, const char* k) {
    xiiRGTextureHandle t;
    bb.TryGetValue(xiiMakeHashedString(k), t);
    if (t.IsValid()) h = builder.ReadTexture(t, xiiGALResourceStateFlags::ShaderResource);
  };
  readTex(data.m_hTransmittanceLUT, xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT);
  readTex(data.m_hMultiScatterLUT, xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT);
  readTex(data.m_hVolumetricScattering, xiiRGBlackboardKeys::k_VolumetricScattering);
  readTex(data.m_hSkyRadiance, xiiRGBlackboardKeys::k_SkyRadiance);

  xiiView::EnsureComputePipeline(lp.m_pAtmosphereCompositePipeline, "Shaders/Pipeline/AtmosphereComposite.xiiShader");
}

static void ExecuteAtmComposite(xiiView& view, const AtmCompositeData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd     = ctx.GetCommandList();
  auto&              lp      = view.m_ViewPassResources.m_LightingPasses;
  auto               bindSRV = [&](const char* s, xiiRGTextureHandle h) {
    if (h.IsValid()) cmd.ResolveAndSetShaderResourceView(s, ctx.GetTexture(h)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  };
  cmd.BeginDebugGroup("AtmosphereComposite");
  cmd.SetPipelineState(lp.m_pAtmosphereCompositePipeline);
  bindSRV("g_Transmittance", data.m_hTransmittanceLUT);
  bindSRV("g_MultiScatter", data.m_hMultiScatterLUT);
  bindSRV("g_VolumetricFog", data.m_hVolumetricScattering);
  bindSRV("g_PrevSkyRadiance", data.m_hSkyRadiance);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// BuildStage6_MainLighting - entry point
//

void xiiView::BuildStage6_MainLighting(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  graph.AddPass<DirectLightData>(
    "DeferredDirectLighting", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](DirectLightData& d, xiiRGBuilder& b) { SetupDirectLighting(*self, d, b, blackboard); },
    [self](const DirectLightData& d, xiiRGPassContext& c) { ExecuteDirectLighting(*self, d, c); });

  graph.AddPass<IndirectLightData>(
    "DeferredIndirectLighting", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](IndirectLightData& d, xiiRGBuilder& b) { SetupIndirectLighting(*self, d, b, blackboard); },
    [self](const IndirectLightData& d, xiiRGPassContext& c) { ExecuteIndirectLighting(*self, d, c); });

  graph.AddPass<RTGIData>(
    "RTGIFinalGather", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](RTGIData& d, xiiRGBuilder& b) { SetupRTGI(*self, d, b, blackboard); },
    [self](const RTGIData& d, xiiRGPassContext& c) { ExecuteRTGI(*self, d, c); });

  graph.AddPass<RTReflData>(
    "RTReflections", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](RTReflData& d, xiiRGBuilder& b) { SetupRTReflections(*self, d, b, blackboard); },
    [self](const RTReflData& d, xiiRGPassContext& c) { ExecuteRTReflections(*self, d, c); });

  graph.AddPass<SSRData>(
    "SSR", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](SSRData& d, xiiRGBuilder& b) { SetupSSR(*self, d, b, blackboard); },
    [self](const SSRData& d, xiiRGPassContext& c) { ExecuteSSR(*self, d, c); });

  graph.AddPass<VolumetricIntegrateData>(
    "VolumetricFogIntegrate", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](VolumetricIntegrateData& d, xiiRGBuilder& b) { SetupVolumetricIntegrate(*self, d, b, blackboard); },
    [self](const VolumetricIntegrateData& d, xiiRGPassContext& c) { ExecuteVolumetricIntegrate(*self, d, c); });

  graph.AddPass<VolumetricTemporalData>(
    "VolumetricFogTemporalRep", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](VolumetricTemporalData& d, xiiRGBuilder& b) { SetupVolumetricTemporal(*self, d, b, blackboard); },
    [self](const VolumetricTemporalData& d, xiiRGPassContext& c) { ExecuteVolumetricTemporal(*self, d, c); });

  graph.AddPass<AtmCompositeData>(
    "VolumetricLightAccumulate", xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](AtmCompositeData& d, xiiRGBuilder& b) { SetupAtmComposite(*self, d, b, blackboard); },
    [self](const AtmCompositeData& d, xiiRGPassContext& c) { ExecuteAtmComposite(*self, d, c); });
}
