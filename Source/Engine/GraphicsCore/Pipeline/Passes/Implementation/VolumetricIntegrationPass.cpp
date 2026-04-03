#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/VolumetricIntegrationPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVolumetricIntegrationPass, 1, xiiRTTIDefaultAllocator<xiiVolumetricIntegrationPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("VolumetricIntegrationPass")), XII_MEMBER_PROPERTY("ScatterCoeff", m_fScatteringCoefficient)->AddAttributes(new xiiDefaultValueAttribute(0.02f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiVolumetricIntegrationPass::xiiVolumetricIntegrationPass() : xiiRenderPipelinePass("VolumetricIntegrationPass") {}
xiiVolumetricIntegrationPass::~xiiVolumetricIntegrationPass() = default;

namespace { struct VolIntData { xiiRGTextureHandle hFroxelScattering, hDepth, hVolumetricOut; xiiUInt32 cX = 160u, cY = 90u, cZ = 64u; }; }

void xiiVolumetricIntegrationPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hFroxel, hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), hFroxel);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),      hDepth);

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiGALTextureCreationDescription outDesc;
  outDesc.m_uiWidth = uiW; outDesc.m_uiHeight = uiH; outDesc.m_uiMipLevels = 1u;
  outDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  outDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<VolIntData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hFroxel, hDepth, outDesc, uiW, uiH](VolIntData& data, xiiRGBuilder& builder)
    {
      if (hFroxel.IsValid()) data.hFroxelScattering = builder.ReadTexture(hFroxel, xiiGALResourceStateFlags::ShaderResource);
      if (hDepth.IsValid())  data.hDepth             = builder.ReadTexture(hDepth,  xiiGALResourceStateFlags::ShaderResource);
      data.hVolumetricOut = builder.WriteTexture("VolumetricScattering", outDesc, xiiGALResourceStateFlags::UnorderedAccess);
    },
    [](const VolIntData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Volumetric Integration");
      // VolumetricLightIntegration.xiiShader: ray-march each froxel column, accumulate front-to-back.
      cmd.Dispatch((data.cX + 7u) / 8u, (data.cY + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VolumetricScattering), pData->hVolumetricOut);
}
