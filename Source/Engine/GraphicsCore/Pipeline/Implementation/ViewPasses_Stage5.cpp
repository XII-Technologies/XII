// XII Engine - ViewPasses_Stage5.cpp
// Stage 5: Lighting preparation (all compute).
// BRDF LUT (once) → Atmosphere LUTs (once) → Sky irradiance → Reflection probe conv →
// Volumetric fog init → DDGI probe gather → GTAO → GTAO denoise.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

//
// BRDF LUT generation (once, persisted across frames)
//
namespace
{
  struct BRDFLutData
  {
    xiiRGTextureHandle m_hBRDFLut;
    bool               m_bNeedsGeneration = false;
  };
} // namespace

static void SetupBRDFLut(xiiView& view, BRDFLutData& data, xiiRGBuilder& builder)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  if (!lp.m_pBRDFLut)
  {
    // First frame - create the persistent 256×256 R16G16F texture.
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType      = xiiGALTextureType::Texture2D;
    desc.m_Format           = xiiGALTextureFormat::RG16Float;
    desc.m_uiWidth          = 256u;
    desc.m_uiHeight         = 256u;
    desc.m_uiMipLevels      = 1u;
    desc.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Usage            = xiiGALResourceUsage::Default;
    lp.m_pBRDFLut           = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
    data.m_bNeedsGeneration = true;
  }

  // Import as persistent resource; mark as UAV only on the generation frame, SRV afterwards.
  data.m_hBRDFLut = builder.ImportTexture(xiiRGBlackboardKeys::k_BRDFLut, lp.m_pBRDFLut,
                                          data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);
  if (data.m_bNeedsGeneration)
    data.m_hBRDFLut = builder.WriteTexture(data.m_hBRDFLut, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pBRDFLutPipeline, "Shaders/Pipeline/BRDFLUTGenerate.xiiShader");
}

static void ExecuteBRDFLut(xiiView& view, const BRDFLutData& data, xiiRGPassContext& ctx)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("BRDFLUTGenerate");
  cmd.SetPipelineState(lp.m_pBRDFLutPipeline);
  cmd.ResolveAndSetUnorderedAccessView("g_BRDFLutOut", ctx.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({32u, 32u, 1u}); // 256/8 × 256/8
  lp.m_bBRDFLutGenerated = true;
  cmd.EndDebugGroup();
}

//
// Atmosphere transmittance LUT (once, persistent)
//
namespace
{
  struct AtmTransmittanceData
  {
    xiiRGTextureHandle m_hTransmittanceLUT;
    bool               m_bNeedsGeneration = false;
  };
} // namespace

static void SetupAtmTransmittance(xiiView& view, AtmTransmittanceData& data, xiiRGBuilder& builder)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  if (!lp.m_pAtmTransmittanceLUT)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType        = xiiGALTextureType::Texture2D;
    desc.m_Format             = xiiGALTextureFormat::RGBA16Float;
    desc.m_uiWidth            = 256u;
    desc.m_uiHeight           = 64u;
    desc.m_uiMipLevels        = 1u;
    desc.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Usage              = xiiGALResourceUsage::Default;
    lp.m_pAtmTransmittanceLUT = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
    data.m_bNeedsGeneration   = true;
  }

  data.m_hTransmittanceLUT = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, lp.m_pAtmTransmittanceLUT,
                                                   data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);
  if (data.m_bNeedsGeneration)
    data.m_hTransmittanceLUT = builder.WriteTexture(data.m_hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pAtmTransmittancePipeline, "Shaders/Pipeline/AtmosphereTransmittance.xiiShader");
}

static void ExecuteAtmTransmittance(xiiView& view, const AtmTransmittanceData& data, xiiRGPassContext& ctx)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("AtmosphereTransmittanceLUT");
  cmd.SetPipelineState(lp.m_pAtmTransmittancePipeline);
  cmd.ResolveAndSetUnorderedAccessView("g_TransmittanceOut", ctx.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({32u, 8u, 1u}); // 256/8 × 64/8
  cmd.EndDebugGroup();
}

//
// Atmosphere multiple-scattering LUT (once, persistent)
//
namespace
{
  struct AtmMultiScatterData
  {
    xiiRGTextureHandle m_hMultiScatterLUT;
    xiiRGTextureHandle m_hTransmittanceLUT;
    bool               m_bNeedsGeneration = false;
  };
} // namespace

static void SetupAtmMultiScatter(xiiView& view, AtmMultiScatterData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  if (!lp.m_pAtmMultiScatterLUT)
  {
    xiiGALTextureCreationDescription desc;
    desc.m_TextureType       = xiiGALTextureType::Texture2D;
    desc.m_Format            = xiiGALTextureFormat::RGBA16Float;
    desc.m_uiWidth           = 32u;
    desc.m_uiHeight          = 32u;
    desc.m_uiMipLevels       = 1u;
    desc.m_BindFlags         = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_Usage             = xiiGALResourceUsage::Default;
    lp.m_pAtmMultiScatterLUT = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
    data.m_bNeedsGeneration  = true;
  }

  xiiRGTextureHandle hTrans;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), hTrans);
  if (hTrans.IsValid())
    data.m_hTransmittanceLUT = builder.ReadTexture(hTrans, xiiGALResourceStateFlags::ShaderResource);

  data.m_hMultiScatterLUT = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, lp.m_pAtmMultiScatterLUT,
                                                  data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);
  if (data.m_bNeedsGeneration)
    data.m_hMultiScatterLUT = builder.WriteTexture(data.m_hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pAtmMultiScatterPipeline, "Shaders/Pipeline/AtmosphereMultiScatter.xiiShader");
}

static void ExecuteAtmMultiScatter(xiiView& view, const AtmMultiScatterData& data, xiiRGPassContext& ctx)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("AtmosphereMultiScatterLUT");
  cmd.SetPipelineState(lp.m_pAtmMultiScatterPipeline);
  if (data.m_hTransmittanceLUT.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_Transmittance", ctx.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_MultiScatterOut", ctx.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({4u, 4u, 1u}); // 32/8 × 32/8
  lp.m_bAtmLutsGenerated = true;
  cmd.EndDebugGroup();
}

//
// Sky irradiance convolution
//
namespace
{
  struct SkyIrradianceData
  {
    xiiRGTextureHandle m_hTransmittanceLUT;
    xiiRGTextureHandle m_hMultiScatterLUT;
    xiiRGTextureHandle m_hSkyRadiance;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupSkyIrradiance(xiiView& view, SkyIrradianceData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hTrans, hMulti;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), hTrans);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT), hMulti);
  if (hTrans.IsValid()) data.m_hTransmittanceLUT = builder.ReadTexture(hTrans, xiiGALResourceStateFlags::ShaderResource);
  if (hMulti.IsValid()) data.m_hMultiScatterLUT = builder.ReadTexture(hMulti, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType  = xiiGALTextureType::Texture2D;
  desc.m_Format       = xiiGALTextureFormat::RGBA16Float;
  desc.m_uiWidth      = data.m_uiRenderW;
  desc.m_uiHeight     = data.m_uiRenderH;
  desc.m_uiMipLevels  = 1u;
  desc.m_BindFlags    = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage        = xiiGALResourceUsage::Default;
  data.m_hSkyRadiance = builder.WriteTexture(xiiRGBlackboardKeys::k_SkyRadiance, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pSkyIrradiancePipeline, "Shaders/Pipeline/ReflectionIrradiance.xiiShader");
}

static void ExecuteSkyIrradiance(xiiView& view, const SkyIrradianceData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("SkyIrradianceConv");
  cmd.SetPipelineState(lp.m_pSkyIrradiancePipeline);
  if (data.m_hTransmittanceLUT.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_Transmittance", ctx.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hMultiScatterLUT.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_MultiScatter", ctx.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_SkyOut", ctx.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Reflection probe filtered specular convolution
//
namespace
{
  struct ReflProbeConvData
  {
    xiiRGBufferHandle  m_hProbeMask;
    xiiRGTextureHandle m_hBRDFLut;
  };
} // namespace

static void SetupReflProbeConv(xiiView& view, ReflProbeConvData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  xiiRGBufferHandle hMask;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_ReflectionProbeMask), hMask);
  if (hMask.IsValid())
    data.m_hProbeMask = builder.ReadBuffer(hMask, xiiGALResourceStateFlags::ShaderResource);

  xiiRGTextureHandle hBRDF;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_BRDFLut), hBRDF);
  if (hBRDF.IsValid())
    data.m_hBRDFLut = builder.ReadTexture(hBRDF, xiiGALResourceStateFlags::ShaderResource);

  xiiView::EnsureComputePipeline(lp.m_pReflProbeConvPipeline, "Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
}

static void ExecuteReflProbeConv(xiiView& view, const ReflProbeConvData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("ReflectionProbeConv");
  cmd.SetPipelineState(lp.m_pReflProbeConvPipeline);
  if (data.m_hBRDFLut.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_BRDFLut", ctx.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hProbeMask.IsValid())
    cmd.ResolveAndSetShaderResourceBufferView("g_ProbeMask", ctx.GetBuffer(data.m_hProbeMask)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({8u, 8u, 6u}); // 6 faces × 8×8 mip dispatch
  cmd.EndDebugGroup();
}

//
// Volumetric fog froxel init (second init pass after Stage-1 allocation)
//
namespace
{
  struct FroxelFogInitData
  {
    xiiRGBufferHandle  m_hFroxelMetadata;
    xiiRGTextureHandle m_hFroxelScattering;
  };
} // namespace

static void SetupFroxelFogInit(xiiView& view, FroxelFogInitData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& lp = view.m_ViewPassResources.m_LightingPrepPasses;

  xiiRGBufferHandle hMeta;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelMetadataBuffer), hMeta);
  if (hMeta.IsValid())
    data.m_hFroxelMetadata = builder.ReadBuffer(hMeta, xiiGALResourceStateFlags::ShaderResource);

  xiiRGTextureHandle hScat;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), hScat);
  if (hScat.IsValid())
    data.m_hFroxelScattering = builder.WriteTexture(hScat, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(lp.m_pFroxelFogInitPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

static void ExecuteFroxelFogInit(xiiView& view, const FroxelFogInitData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              lp  = view.m_ViewPassResources.m_LightingPrepPasses;

  cmd.BeginDebugGroup("VolumetricFogInit");
  cmd.SetPipelineState(lp.m_pFroxelFogInitPipeline);
  if (data.m_hFroxelMetadata.IsValid())
    cmd.ResolveAndSetShaderResourceBufferView("g_FroxelMeta", ctx.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  if (data.m_hFroxelScattering.IsValid())
    cmd.ResolveAndSetUnorderedAccessView("g_FroxelScatterOut", ctx.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({16u, 9u, 8u});
  cmd.EndDebugGroup();
}

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
