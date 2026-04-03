#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/EmissiveAuxPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEmissiveAuxPass, 1, xiiRTTIDefaultAllocator<xiiEmissiveAuxPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("EmissiveAuxPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiEmissiveAuxPass::xiiEmissiveAuxPass() : xiiRenderPipelinePass("EmissiveAuxPass") {}
xiiEmissiveAuxPass::~xiiEmissiveAuxPass() = default;

namespace { struct EmissiveData { xiiRGTextureHandle hDepth, hDrawArgs, hEmissive; xiiRGBufferHandle hArgs; xiiUInt32 uiW = 1u, uiH = 1u; }; }

void xiiEmissiveAuxPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Emissive target already created by GBufferBasePass — read+write to append.
  xiiRGTextureHandle hEmissive; xiiRGTextureHandle hDepth; xiiRGBufferHandle hArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive),      hEmissive);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hArgs);

  auto [pData, hPass] = graph.AddPass<EmissiveData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hEmissive, hDepth, hArgs, uiW, uiH](EmissiveData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::DepthRead);
      if (hArgs.IsValid())     data.hArgs     = builder.ReadBuffer(hArgs,      xiiGALResourceStateFlags::IndirectArgument);
      // Additive write into the existing emissive target.
      if (hEmissive.IsValid()) data.hEmissive = builder.WriteTexture(hEmissive, xiiGALResourceStateFlags::RenderTarget);
      data.uiW = uiW; data.uiH = uiH;
    },
    [](const EmissiveData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Emissive + Aux Materials");
      // EmissiveMaterial.xiiShader: indirect draw for emissive-only material bins (additive blend).
      if (auto* pArgs = context.GetBuffer(data.hArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    }
  );

  // hEmissive handle remains the same — no need to re-publish (same texture resource).
}
