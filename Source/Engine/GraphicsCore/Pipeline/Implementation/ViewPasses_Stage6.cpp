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
