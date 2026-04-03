#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/AtmosphereCompositePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereCompositePass, 1, xiiRTTIDefaultAllocator<xiiAtmosphereCompositePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("AtmosphereCompositePass")), XII_MEMBER_PROPERTY("SunSolidAngle", m_fSunSolidAngle)->AddAttributes(new xiiDefaultValueAttribute(6.8e-5f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiAtmosphereCompositePass::xiiAtmosphereCompositePass() : xiiRenderPipelinePass("AtmosphereCompositePass") {}
xiiAtmosphereCompositePass::~xiiAtmosphereCompositePass() = default;

namespace { struct AtmCompData { xiiRGTextureHandle hDepth, hTransmittance, hMultiScatter, hDirectLight, hSky; xiiUInt32 uiW = 1u, uiH = 1u; float fSunSolidAngle = 6.8e-5f; }; }

void xiiAtmosphereCompositePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hTransmittance, hMultiScatter, hDirectLight;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),         hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), hTransmittance);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT),  hMultiScatter);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer),       hDirectLight);

  xiiGALTextureCreationDescription skyDesc;
  skyDesc.m_uiWidth = uiW; skyDesc.m_uiHeight = uiH; skyDesc.m_uiMipLevels = 1u;
  skyDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  skyDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<AtmCompData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hTransmittance, hMultiScatter, hDirectLight, skyDesc, uiW, uiH, this](AtmCompData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())         data.hDepth         = builder.ReadTexture(hDepth,         xiiGALResourceStateFlags::ShaderResource);
      if (hTransmittance.IsValid()) data.hTransmittance = builder.ReadTexture(hTransmittance, xiiGALResourceStateFlags::ShaderResource);
      if (hMultiScatter.IsValid())  data.hMultiScatter  = builder.ReadTexture(hMultiScatter,  xiiGALResourceStateFlags::ShaderResource);
      if (hDirectLight.IsValid())   data.hDirectLight   = builder.WriteTexture(hDirectLight,  xiiGALResourceStateFlags::UnorderedAccess);
      data.hSky = builder.WriteTexture("SkyRadiance", skyDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.fSunSolidAngle = m_fSunSolidAngle;
    },
    [](const AtmCompData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Atmosphere + Sky Composite");
      // AtmosphereComposite.xiiShader: sky + sun disc into skybox pixels (depth == far plane).
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkyRadiance), pData->hSky);
}
