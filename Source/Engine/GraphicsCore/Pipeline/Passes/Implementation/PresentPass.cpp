#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/PresentPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPresentPass, 1, xiiRTTIDefaultAllocator<xiiPresentPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("PresentPass")), XII_MEMBER_PROPERTY("VSync", m_bVSync)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiPresentPass::xiiPresentPass() : xiiRenderPipelinePass("PresentPass") {}
xiiPresentPass::~xiiPresentPass() = default;

namespace { struct PresentData { xiiRGTextureHandle hFinalColor; bool bVSync = true; }; }

void xiiPresentPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hFinal;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FinalColorTarget), hFinal);

  auto [pData, hPass] = graph.AddPass<PresentData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hFinal, this](PresentData& data, xiiRGBuilder& builder)
    {
      if (hFinal.IsValid())
        data.hFinalColor = builder.ReadTexture(hFinal, xiiGALResourceStateFlags::ShaderResource);
      data.bVSync = m_bVSync;
      // Mark the swapchain RT as a written output so the render graph transitions it correctly.
      builder.WriteSwapchainOutput(xiiGALResourceStateFlags::Present);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const PresentData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Present");
      // FinalBlit.xiiShader: copies final color to the swapchain back buffer in the correct color space.
      // Handles sRGB gamma correction or display mapping for the active output format.
      xiiGALTexture* pFinal = context.GetTexture(data.hFinalColor);
      xiiGALTexture* pBack  = context.GetSwapchainBackBuffer();
      if (pFinal && pBack)
        cmd.CopyTextureRegion(pFinal, nullptr, pBack, nullptr);
      cmd.PopDebugGroup();
      // Pop the outermost "XII Frame" debug group opened by FrameSetupPass.
      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}
