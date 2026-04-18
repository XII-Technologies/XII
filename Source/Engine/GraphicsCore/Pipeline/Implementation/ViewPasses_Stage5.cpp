// XII Engine - ViewPasses_Stage5.cpp
// Stage 5: Lighting preparation (all compute).
// BRDF LUT (once) -> Atmosphere LUTs (once) -> Sky irradiance -> Reflection probe conv ->
// Volumetric fog init -> DDGI probe gather -> GTAO -> GTAO denoise.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

//
// DDGI final gather
//
namespace
{
  struct DDGIData
  {
    xiiRGTextureHandle m_hDDGIIrradiance;
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hGBufferNormal;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupDDGI(xiiView& view, DDGIData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth, hNormal;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal), hNormal);
  if (hDepth.IsValid()) data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
  if (hNormal.IsValid()) data.m_hGBufferNormal = builder.ReadTexture(hNormal, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType     = xiiGALTextureType::Texture2D;
  desc.m_Format          = xiiGALTextureFormat::RGBA16Float;
  desc.m_uiWidth         = data.m_uiRenderW;
  desc.m_uiHeight        = data.m_uiRenderH;
  desc.m_uiMipLevels     = 1u;
  desc.m_BindFlags       = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage           = xiiGALResourceUsage::Default;
  data.m_hDDGIIrradiance = builder.WriteTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pDDGIProbePipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

static void ExecuteDDGI(xiiView& view, const DDGIData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("DDGIProbeSample");
  cmd.SetPipelineState(lp.m_pDDGIProbePipeline);
  if (data.m_hSceneDepth.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_SceneDepth", ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hGBufferNormal.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_GBufNormal", ctx.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_DDGIOut", ctx.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// GTAO (ground truth ambient occlusion)
//
namespace
{
  struct GTAOData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hNormalRoughness;
    xiiRGTextureHandle m_hRawAO;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupGTAO(xiiView& view, GTAOData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth, hNR;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  if (hDepth.IsValid()) data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
  if (hNR.IsValid()) data.m_hNormalRoughness = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth     = data.m_uiRenderW;
  desc.m_uiHeight    = data.m_uiRenderH;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRawAO      = builder.WriteTexture(xiiRGBlackboardKeys::k_RawAOTexture, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pGTAOPipeline, "Shaders/Pipeline/GTAO.xiiShader");
}

static void ExecuteGTAO(xiiView& view, const GTAOData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("GTAO");
  cmd.SetPipelineState(lp.m_pGTAOPipeline);
  if (data.m_hSceneDepth.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_SceneDepth", ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hNormalRoughness.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_NormalRoughness", ctx.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_AOOut", ctx.GetTexture(data.m_hRawAO)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// GTAO denoise (bilateral blur on raw AO)
//
namespace
{
  struct GTAODenoiseData
  {
    xiiRGTextureHandle m_hRawAO;
    xiiRGTextureHandle m_hStableAO;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupGTAODenoise(xiiView& view, GTAODenoiseData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hRaw;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RawAOTexture), hRaw);
  if (hRaw.IsValid())
    data.m_hRawAO = builder.ReadTexture(hRaw, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth     = data.m_uiRenderW;
  desc.m_uiHeight    = data.m_uiRenderH;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hStableAO   = builder.WriteTexture(xiiRGBlackboardKeys::k_StableAOTexture, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pGTAODenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

static void ExecuteGTAODenoise(xiiView& view, const GTAODenoiseData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("GTAODenoise");
  cmd.SetPipelineState(lp.m_pGTAODenoisePipeline);
  if (data.m_hRawAO.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_Input", ctx.GetTexture(data.m_hRawAO)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_Output", ctx.GetTexture(data.m_hStableAO)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// BuildStage5_LightingPrep - entry point
//

void xiiView::BuildStage5_LightingPrep(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  // 5a. BRDF LUT (no-op after first frame)
  graph.AddPass<BRDFLutData>(
    "BRDFLUTGenerate",
    xiiGALCommandQueueFlags::Compute,
    [self](BRDFLutData& d, xiiRGBuilder& b) { SetupBRDFLut(*self, d, b); },
    [self](const BRDFLutData& d, xiiRGPassContext& c) { ExecuteBRDFLut(*self, d, c); });

  // 5b. Atmosphere transmittance LUT (no-op after first frame)
  graph.AddPass<AtmTransmittanceData>(
    "AtmosphereTransmittanceLUT",
    xiiGALCommandQueueFlags::Compute,
    [self](AtmTransmittanceData& d, xiiRGBuilder& b) { SetupAtmTransmittance(*self, d, b); },
    [self](const AtmTransmittanceData& d, xiiRGPassContext& c) { ExecuteAtmTransmittance(*self, d, c); });

  // 5c. Atmosphere multiscatter LUT (no-op after first frame)
  graph.AddPass<AtmMultiScatterData>(
    "AtmosphereMultiScatterLUT",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](AtmMultiScatterData& d, xiiRGBuilder& b) { SetupAtmMultiScatter(*self, d, b, blackboard); },
    [self](const AtmMultiScatterData& d, xiiRGPassContext& c) { ExecuteAtmMultiScatter(*self, d, c); });

  // 5d. Sky irradiance convolution
  graph.AddPass<SkyIrradianceData>(
    "SkyIrradianceConv",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](SkyIrradianceData& d, xiiRGBuilder& b) { SetupSkyIrradiance(*self, d, b, blackboard); },
    [self](const SkyIrradianceData& d, xiiRGPassContext& c) { ExecuteSkyIrradiance(*self, d, c); });

  // 5e. Reflection probe specular convolution
  graph.AddPass<ReflProbeConvData>(
    "ReflectionProbeConv",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ReflProbeConvData& d, xiiRGBuilder& b) { SetupReflProbeConv(*self, d, b, blackboard); },
    [self](const ReflProbeConvData& d, xiiRGPassContext& c) { ExecuteReflProbeConv(*self, d, c); });

  // 5f. Volumetric fog froxel init (media assignment)
  graph.AddPass<FroxelFogInitData>(
    "VolumetricFogInit",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](FroxelFogInitData& d, xiiRGBuilder& b) { SetupFroxelFogInit(*self, d, b, blackboard); },
    [self](const FroxelFogInitData& d, xiiRGPassContext& c) { ExecuteFroxelFogInit(*self, d, c); });

  // 5g. DDGI probe irradiance sample
  graph.AddPass<DDGIData>(
    "DDGIProbeSample",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](DDGIData& d, xiiRGBuilder& b) { SetupDDGI(*self, d, b, blackboard); },
    [self](const DDGIData& d, xiiRGPassContext& c) { ExecuteDDGI(*self, d, c); });

  // 5h. GTAO
  graph.AddPass<GTAOData>(
    "GTAO",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](GTAOData& d, xiiRGBuilder& b) { SetupGTAO(*self, d, b, blackboard); },
    [self](const GTAOData& d, xiiRGPassContext& c) { ExecuteGTAO(*self, d, c); });

  // 5i. GTAO denoise
  graph.AddPass<GTAODenoiseData>(
    "GTAODenoise",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](GTAODenoiseData& d, xiiRGBuilder& b) { SetupGTAODenoise(*self, d, b, blackboard); },
    [self](const GTAODenoiseData& d, xiiRGPassContext& c) { ExecuteGTAODenoise(*self, d, c); });
}
