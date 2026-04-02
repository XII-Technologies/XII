#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/RTReflectionPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRTReflectionPass, 1, xiiRTTIDefaultAllocator<xiiRTReflectionPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("RTReflectionPass")), XII_MEMBER_PROPERTY("MaxRays", m_uiMaxRaysPerPixel)->AddAttributes(new xiiDefaultValueAttribute(2u)), XII_MEMBER_PROPERTY("MaxRoughness", m_fMaxRoughnessForRT)->AddAttributes(new xiiDefaultValueAttribute(0.4f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiRTReflAutoReg { xiiRTReflAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiRTReflectionPass)); } }; static xiiRTReflAutoReg s_AutoReg; }

xiiRTReflectionPass::xiiRTReflectionPass() : xiiRenderPipelinePass("RTReflectionPass") {}
xiiRTReflectionPass::~xiiRTReflectionPass() = default;

namespace { struct RTReflData { xiiRGTextureHandle hDepth, hNR, hVelocity, hHDRScene, hSSR, hRawRefl, hFinalRefl; xiiUInt32 uiW = 1u, uiH = 1u, uiMaxRays = 2u; float fMaxRoughness = 0.4f; }; }

void xiiRTReflectionPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!xiiGALDevice::GetDefaultDevice()->GetFeatures().m_bRayTracing)
    return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hVelocity, hSSR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer),        hVelocity);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture),            hSSR);

  xiiGALTextureCreationDescription reflDesc;
  reflDesc.m_uiWidth = uiW; reflDesc.m_uiHeight = uiH; reflDesc.m_uiMipLevels = 1u;
  reflDesc.m_Format   = xiiGALTextureFormat::R16G16B16A16Float;
  reflDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<RTReflData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, hSSR, reflDesc, uiW, uiH, this](RTReflData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid())       data.hNR       = builder.ReadTexture(hNR,       xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      if (hSSR.IsValid())      data.hSSR      = builder.ReadTexture(hSSR,      xiiGALResourceStateFlags::ShaderResource);
      data.hRawRefl   = builder.WriteTexture("RTRawReflections",   reflDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalRefl = builder.WriteTexture("RTFinalReflections", reflDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiMaxRays = m_uiMaxRaysPerPixel; data.fMaxRoughness = m_fMaxRoughnessForRT;
    },
    [](const RTReflData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Reflections");
      // Pass 39: RTReflection.xiiShader (TraceRays) — glossy reflection rays, budget by roughness.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 40: RTReflectionTemporal.xiiShader — radiance-space clamped accumulation.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 41: RTReflectionDenoise.xiiShader — two-stage diffuse/specular split denoise.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawReflections),   pData->hRawRefl);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalReflections), pData->hFinalRefl);
}
