#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameSetupPass, 1, xiiRTTIDefaultAllocator<xiiFrameSetupPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("FrameSetupPass")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiFrameSetupPass::xiiFrameSetupPass() : xiiRenderPipelinePass("FrameSetupPass")
{
}

xiiFrameSetupPass::~xiiFrameSetupPass() = default;

namespace
{
  struct SetupPassData
  {
  };
} // namespace

void xiiFrameSetupPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto [pData, hPass] = graph.AddPass<SetupPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [](SetupPassData& data, xiiRGBuilder& builder) {
      builder.SetPassSideEffects(true); // Must be executed regardless of connections.
      builder.SetPassAllowMerge(false); // Distinct beginning step
    },
    [this](const SetupPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();

      // Emit top-of-frame markers for profilers/GPU debug tools
      cmd.PushDebugGroup("Frame Setup");

      // (Optional future) Upload per-frame HUD stats, start GPU duration timestamp queries across the frame, etc.

      cmd.PopDebugGroup();
    },
    true);
}
