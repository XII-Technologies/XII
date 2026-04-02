#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/TransparentPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparentPass, 1, xiiRTTIDefaultAllocator<xiiTransparentPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("TransparentPass")), XII_MEMBER_PROPERTY("WeightedBlendedOIT", m_bUseWeightedBlendedOIT)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiTranspAutoReg { xiiTranspAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiTransparentPass)); } }; static xiiTranspAutoReg s_AutoReg; }

xiiTransparentPass::xiiTransparentPass() : xiiRenderPipelinePass("TransparentPass") {}
xiiTransparentPass::~xiiTransparentPass() = default;

namespace { struct TransparentData { xiiRGTextureHandle hDepth, hHDRScene; xiiRGBufferHandle hDrawArgs; xiiUInt32 uiW = 1u, uiH = 1u; bool bOIT = true; }; }

void xiiTransparentPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hHDRScene; xiiRGBufferHandle hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor),        hHDRScene);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto [pData, hPass] = graph.AddPass<TransparentData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hHDRScene, hDrawArgs, uiW, uiH, this](TransparentData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::DepthRead);
      if (hHDRScene.IsValid()) data.hHDRScene = builder.WriteTexture(hHDRScene, xiiGALResourceStateFlags::RenderTarget);
      if (hDrawArgs.IsValid()) data.hDrawArgs = builder.ReadBuffer(hDrawArgs,   xiiGALResourceStateFlags::IndirectArgument);
      data.uiW = uiW; data.uiH = uiH; data.bOIT = m_bUseWeightedBlendedOIT;
      builder.SetPassAllowMerge(false);
    },
    [](const TransparentData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Transparent + Refractive");
      // Transparent.xiiShader (OIT) or depth-sorted additive pass.
      if (auto* pArgs = context.GetBuffer(data.hDrawArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    }
  );
}
