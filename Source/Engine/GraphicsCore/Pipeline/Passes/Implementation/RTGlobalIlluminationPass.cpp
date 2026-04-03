#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/RTGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRTGlobalIlluminationPass, 1, xiiRTTIDefaultAllocator<xiiRTGlobalIlluminationPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("RTGlobalIlluminationPass")), XII_MEMBER_PROPERTY("RaysPerPixel", m_uiRaysPerPixel)->AddAttributes(new xiiDefaultValueAttribute(1u)), XII_MEMBER_PROPERTY("ReservoirCandidates", m_uiReservoirCandidateCount)->AddAttributes(new xiiDefaultValueAttribute(8u)), XII_MEMBER_PROPERTY("MaxBounces", m_uiMaxBounces)->AddAttributes(new xiiDefaultValueAttribute(1u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiRTGlobalIlluminationPass::xiiRTGlobalIlluminationPass() : xiiRenderPipelinePass("RTGlobalIlluminationPass") {}
xiiRTGlobalIlluminationPass::~xiiRTGlobalIlluminationPass() = default;

namespace { struct RTGIData { xiiRGTextureHandle hDepth, hNR, hVelocity, hRawGI, hFinalGI; xiiUInt32 uiW = 1u, uiH = 1u, uiRPP = 1u, uiCandidates = 8u; }; }

void xiiRTGlobalIlluminationPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
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

  xiiGALTextureCreationDescription giDesc;
  giDesc.m_uiWidth = uiW; giDesc.m_uiHeight = uiH; giDesc.m_uiMipLevels = 1u;
  giDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  giDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<RTGIData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, giDesc, uiW, uiH, this](RTGIData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid())       data.hNR       = builder.ReadTexture(hNR,       xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawGI   = builder.WriteTexture("RTRawGI",   giDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalGI = builder.WriteTexture("RTFinalGI", giDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiRPP = m_uiRaysPerPixel; data.uiCandidates = m_uiReservoirCandidateCount;
    },
    [](const RTGIData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Global Illumination");
      // Pass 42: RTGIFinalGather.xiiShader (TraceRays) — ReSTIR reservoir resampling initial candidates.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 43: RTGITemporal.xiiShader — temporal reservoir reuse + firefly clamp.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 44: RTGIDenoise.xiiShader — diffuse/specular indirect denoiser.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawGI),   pData->hRawGI);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalGI), pData->hFinalGI);
}
