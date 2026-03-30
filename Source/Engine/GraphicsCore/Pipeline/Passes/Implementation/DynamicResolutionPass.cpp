#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicResolutionPass, 1, xiiRTTIDefaultAllocator<xiiDynamicResolutionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("DynamicResolutionPass")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDynamicResolutionPass::xiiDynamicResolutionPass() : xiiRenderPipelinePass("DynamicResolutionPass")
{
}

xiiDynamicResolutionPass::~xiiDynamicResolutionPass() = default;

namespace
{
  struct DynResPassData
  {
  };
}

void xiiDynamicResolutionPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // CPU-side heuristic execution: Look at previous frame times and adjust scale.
  // For now, we simulate a simple heuristic or stick to 1.0f.
  m_fCurrentScale = 1.0f; // Placeholder for actual GPU frame time logic

  // Publish to blackboard immediately during graph setup so downstream setup steps can size their render targets.
  // Set is not thread-safe and must be done before Execute(), which perfectly fits graph compilation/setup logic.
  blackboard.Set(xiiMakeHashedString("DynamicResolutionScale"), m_fCurrentScale);

  // Add a GPU marker pass mostly for profiling consistency.
  graph.AddPass<DynResPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [](DynResPassData& data, xiiRGBuilder& builder)
    {
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this](const DynResPassData& data, xiiRGPassContext& context)
    {
      // The resolution was evaluated before graph execution, so there is no GPU workload here.
      // This is purely for the graph topological layout if needed.
    }
  );
}
