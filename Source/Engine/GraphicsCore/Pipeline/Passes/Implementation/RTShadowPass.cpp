#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/RTShadowPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRTShadowPass, 1, xiiRTTIDefaultAllocator<xiiRTShadowPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("RTShadowPass")), XII_MEMBER_PROPERTY("RaysPerPixel", m_uiRaysPerPixel)->AddAttributes(new xiiDefaultValueAttribute(1u)), XII_MEMBER_PROPERTY("LightRadius", m_fLightRadius)->AddAttributes(new xiiDefaultValueAttribute(0.05f)), XII_MEMBER_PROPERTY("TemporalHistory", m_uiTemporalHistoryLen)->AddAttributes(new xiiDefaultValueAttribute(16u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiRTShadowPass::xiiRTShadowPass() : xiiRenderPipelinePass("RTShadowPass") {}
xiiRTShadowPass::~xiiRTShadowPass() = default;

namespace { struct RTShadowData { xiiRGTextureHandle hDepth, hNR, hVelocity, hRawMask, hFinalMask; xiiUInt32 uiW = 1u, uiH = 1u, uiRPP = 1u, uiHistory = 16u; float fLightRadius = 0.05f; }; }

void xiiRTShadowPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!xiiGALDevice::GetDefaultDevice()->GetFeatures().m_bRayTracing)
    return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hVelocity;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer),        hVelocity);

  auto makeR8 = [&](const char* name) {
    xiiGALTextureCreationDescription d; d.m_uiWidth = uiW; d.m_uiHeight = uiH; d.m_uiMipLevels = 1u;
    d.m_Format = xiiGALTextureFormat::R8UNorm;
    d.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto [pData, hPass] = graph.AddPass<RTShadowData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, uiW, uiH, this, makeR8](RTShadowData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid())       data.hNR       = builder.ReadTexture(hNR,       xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawMask   = builder.WriteTexture("RTRawShadows",   makeR8("RTRawShadows"),   xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalMask = builder.WriteTexture("RTFinalShadows", makeR8("RTFinalShadows"), xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH;
      data.uiRPP = m_uiRaysPerPixel; data.uiHistory = m_uiTemporalHistoryLen; data.fLightRadius = m_fLightRadius;
    },
    [](const RTShadowData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Shadows");
      // Pass 36: RTShadow.xiiShader (TraceRays) — m_uiRaysPerPixel per pixel, jittered soft shadow.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 37: RTShadowTemporalAccumulate.xiiShader — reproject + history clamping.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 38: RTShadowSpatialDenoise.xiiShader — normal+depth gated bilateral filter.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawShadowMask),   pData->hRawMask);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalShadowMask), pData->hFinalMask);
}
