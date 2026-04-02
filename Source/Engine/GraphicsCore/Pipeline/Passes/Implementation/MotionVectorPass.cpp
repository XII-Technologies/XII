#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/MotionVectorPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMotionVectorPass, 1, xiiRTTIDefaultAllocator<xiiMotionVectorPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("MotionVectorPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiMotionVecAutoReg { xiiMotionVecAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiMotionVectorPass)); } }; static xiiMotionVecAutoReg s_AutoReg; }

xiiMotionVectorPass::xiiMotionVectorPass() : xiiRenderPipelinePass("MotionVectorPass") {}
xiiMotionVectorPass::~xiiMotionVectorPass() = default;

namespace { struct MotionVecData { xiiRGTextureHandle hDepth; xiiRGTextureHandle hVelocity; xiiUInt32 uiWidth = 1u, uiHeight = 1u; }; }

void xiiMotionVectorPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription velDesc;
  velDesc.m_uiWidth = uiW; velDesc.m_uiHeight = uiH; velDesc.m_uiMipLevels = 1u;
  velDesc.m_Format  = xiiGALTextureFormat::R16G16Float; // screen-space velocity XY
  velDesc.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<MotionVecData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, velDesc, uiW, uiH](MotionVecData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      data.hVelocity = builder.WriteTexture("VelocityBuffer", velDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiWidth = uiW; data.uiHeight = uiH;
    },
    [](const MotionVecData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Motion Vectors");
      // MotionVectors.xiiShader: fullscreen triangle, reads depth + previous VP matrix, outputs (cur-prev) NDC.
      cmd.Draw(3u, 1u, 0u, 0u); // fullscreen triangle
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), pData->hVelocity);
}
