#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/AtmosphereLUTPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereLUTPass, 1, xiiRTTIDefaultAllocator<xiiAtmosphereLUTPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("AtmosphereLUTPass")), XII_MEMBER_PROPERTY("PlanetRadius", m_fPlanetRadius)->AddAttributes(new xiiDefaultValueAttribute(6360.0f)), XII_MEMBER_PROPERTY("AtmosphereRadius", m_fAtmosphereRadius)->AddAttributes(new xiiDefaultValueAttribute(6460.0f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiAtmosphereAutoReg { xiiAtmosphereAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiAtmosphereLUTPass)); } }; static xiiAtmosphereAutoReg s_AutoReg; }

xiiAtmosphereLUTPass::xiiAtmosphereLUTPass() : xiiRenderPipelinePass("AtmosphereLUTPass") {}
xiiAtmosphereLUTPass::~xiiAtmosphereLUTPass() = default;

namespace
{
  struct alignas(16) AtmosphereConstants
  {
    float RayleighScaleHeight; float MieScaleHeight; float MieAnisotropy; float PlanetRadiusKm;
    float AtmosphereRadiusKm;  float _pad[3];
  };
  struct AtmLUTData { xiiRGTextureHandle hTransmittanceLUT; xiiRGTextureHandle hMultiScatterLUT; };
}

void xiiAtmosphereLUTPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_pTransmittanceLUT)
  {
    xiiGALTextureCreationDescription td;
    td.m_uiWidth = 256u; td.m_uiHeight = 64u; td.m_uiMipLevels = 1u;
    td.m_Format = xiiGALTextureFormat::R16G16B16A16Float;
    td.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    m_pTransmittanceLUT = pDevice->CreateTexture(td);
  }
  if (!m_pMultiScatterLUT)
  {
    xiiGALTextureCreationDescription td;
    td.m_uiWidth = 32u; td.m_uiHeight = 32u; td.m_uiMipLevels = 1u;
    td.m_Format = xiiGALTextureFormat::R16G16B16A16Float;
    td.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    m_pMultiScatterLUT = pDevice->CreateTexture(td);
  }

  const AtmosphereConstants consts = { m_fRayleighScaleHeight, m_fMieScaleHeight, m_fMieAnisotropy, m_fPlanetRadius, m_fAtmosphereRadius };

  auto [pData, hPass] = graph.AddPass<AtmLUTData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this](AtmLUTData& data, xiiRGBuilder& builder)
    {
      data.hTransmittanceLUT = builder.ImportTexture("AtmTransmittance", m_pTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
      data.hTransmittanceLUT = builder.WriteTexture(data.hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
      data.hMultiScatterLUT  = builder.ImportTexture("AtmMultiScatter",  m_pMultiScatterLUT,  xiiGALResourceStateFlags::UnorderedAccess);
      data.hMultiScatterLUT  = builder.WriteTexture(data.hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true); // Persistent LUTs
    },
    [consts](const AtmLUTData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Atmosphere LUT Update");
      // AtmosphereTransmittance.xiiShader: 256x64 compute — precomputed optical depth integral.
      cmd.Dispatch((256u + 7u) / 8u, (64u + 7u) / 8u, 1u);
      // AtmosphereMultiScatter.xiiShader: 32x32 compute with multiple scattering approximation.
      cmd.Dispatch((32u + 7u) / 8u,  (32u + 7u) / 8u,  1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), pData->hTransmittanceLUT);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT),  pData->hMultiScatterLUT);
}
