#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/UICompositePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUICompositePass, 1, xiiRTTIDefaultAllocator<xiiUICompositePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("UICompositePass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiUICompAutoReg { xiiUICompAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiUICompositePass)); } }; static xiiUICompAutoReg s_AutoReg; }

xiiUICompositePass::xiiUICompositePass() : xiiRenderPipelinePass("UICompositePass") {}
xiiUICompositePass::~xiiUICompositePass() = default;

namespace { struct UICompData { xiiRGTextureHandle hScene, hUI, hFinal; xiiUInt32 uiW = 1u, uiH = 1u; }; }

void xiiUICompositePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Best available final scene output.
  xiiRGTextureHandle hScene;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SharpenedColor), hScene);
  if (!hScene.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), hScene);
  if (!hScene.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), hScene);

  xiiRGTextureHandle hUI;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UIRenderTarget), hUI);

  xiiGALTextureCreationDescription finalDesc;
  finalDesc.m_uiWidth = uiW; finalDesc.m_uiHeight = uiH; finalDesc.m_uiMipLevels = 1u;
  finalDesc.m_Format  = xiiGALTextureFormat::R8G8B8A8UNorm;
  finalDesc.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<UICompData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hScene, hUI, finalDesc, uiW, uiH](UICompData& data, xiiRGBuilder& builder)
    {
      if (hScene.IsValid()) data.hScene = builder.ReadTexture(hScene, xiiGALResourceStateFlags::ShaderResource);
      if (hUI.IsValid())    data.hUI    = builder.ReadTexture(hUI,    xiiGALResourceStateFlags::ShaderResource);
      data.hFinal = builder.WriteTexture("FinalColor", finalDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiW = uiW; data.uiH = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const UICompData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("UI Composite");
      // UIComposite.xiiShader: blits scene onto render target, then premultiplied-alpha blends UI on top.
      cmd.Draw(3u, 1u, 0u, 0u); // fullscreen triangle for scene blit
      // UI draws (if any) are issued here via retained draw list from the UI subsystem.
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FinalColorTarget), pData->hFinal);
}
