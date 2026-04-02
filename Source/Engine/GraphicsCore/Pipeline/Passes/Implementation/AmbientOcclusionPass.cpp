#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAmbientOcclusionPass, 1, xiiRTTIDefaultAllocator<xiiAmbientOcclusionPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("AmbientOcclusionPass")), XII_MEMBER_PROPERTY("SliceCount", m_uiSliceCount)->AddAttributes(new xiiDefaultValueAttribute(3u)), XII_MEMBER_PROPERTY("StepsPerSlice", m_uiStepsPerSlice)->AddAttributes(new xiiDefaultValueAttribute(4u)), XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.6f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiAOAutoReg { xiiAOAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiAmbientOcclusionPass)); } }; static xiiAOAutoReg s_AutoReg; }

xiiAmbientOcclusionPass::xiiAmbientOcclusionPass() : xiiRenderPipelinePass("AmbientOcclusionPass") {}
xiiAmbientOcclusionPass::~xiiAmbientOcclusionPass() = default;

namespace { struct AOData { xiiRGTextureHandle hDepth, hNR, hHiZ, hRawAO, hStableAO, hVelocity; xiiUInt32 uiW = 1u, uiH = 1u, uiSlices = 3u, uiSteps = 4u; float fRadius = 0.6f; }; }

void xiiAmbientOcclusionPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hHiZ, hVelocity;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid),            hHiZ);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer),         hVelocity);

  auto makeAO = [&](const char* name) {
    xiiGALTextureCreationDescription d; d.m_uiWidth = uiW; d.m_uiHeight = uiH; d.m_uiMipLevels = 1u;
    d.m_Format = xiiGALTextureFormat::R8UNorm;
    d.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto [pData, hPass] = graph.AddPass<AOData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hHiZ, hVelocity, uiW, uiH, this, makeAO](AOData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid())       data.hNR       = builder.ReadTexture(hNR,       xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid())      data.hHiZ      = builder.ReadTexture(hHiZ,      xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawAO    = builder.WriteTexture("RawAO",    makeAO("RawAO"),    xiiGALResourceStateFlags::UnorderedAccess);
      data.hStableAO = builder.WriteTexture("StableAO", makeAO("StableAO"), xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiSlices = m_uiSliceCount; data.uiSteps = m_uiStepsPerSlice; data.fRadius = m_fRadius;
    },
    [](const AOData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("GTAO + Denoise");
      // Pass 29: GTAO.xiiShader — horizon-based AO integration with m_uiSliceCount * m_uiStepsPerSlice samples.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 30: AOSpatialDenoise.xiiShader + AOTemporalDenoise.xiiShader
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RawAOTexture),    pData->hRawAO);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_StableAOTexture), pData->hStableAO);
}
