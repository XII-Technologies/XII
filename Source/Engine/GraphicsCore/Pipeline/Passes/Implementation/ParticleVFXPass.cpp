#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ParticleVFXPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleVFXPass, 1, xiiRTTIDefaultAllocator<xiiParticleVFXPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ParticleVFXPass")), XII_MEMBER_PROPERTY("MaxParticles", m_uiMaxParticles)->AddAttributes(new xiiDefaultValueAttribute(1u << 20u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiParticleAutoReg { xiiParticleAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiParticleVFXPass)); } }; static xiiParticleAutoReg s_AutoReg; }

xiiParticleVFXPass::xiiParticleVFXPass() : xiiRenderPipelinePass("ParticleVFXPass") {}
xiiParticleVFXPass::~xiiParticleVFXPass() = default;

namespace { struct ParticleData { xiiRGTextureHandle hDepth, hHDRScene; xiiUInt32 uiW = 1u, uiH = 1u, uiMaxParticles = 1u << 20u; }; }

void xiiParticleVFXPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hHDRScene;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor),     hHDRScene);

  auto [pData, hPass] = graph.AddPass<ParticleData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hHDRScene, uiW, uiH, this](ParticleData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      if (hHDRScene.IsValid()) data.hHDRScene = builder.WriteTexture(hHDRScene, xiiGALResourceStateFlags::RenderTarget);
      data.uiW = uiW; data.uiH = uiH; data.uiMaxParticles = m_uiMaxParticles;
      builder.SetPassAllowMerge(false);
    },
    [](const ParticleData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Particle VFX");
      // Simulate: ParticleSimulate.xiiShader — position + velocity integration.
      cmd.Dispatch((data.uiMaxParticles + 255u) / 256u, 1u, 1u);
      // Render: Particle.xiiShader — billboard quads, depth-tested, soft particle edge fading.
      cmd.DrawIndirect(nullptr, 0u);
      cmd.PopDebugGroup();
    }
  );
}
