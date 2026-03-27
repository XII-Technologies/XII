#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

// ============================================================================
//  xiiRenderGraphDebug::DumpToDot
// ============================================================================

// static
xiiResult xiiRenderGraphDebug::DumpToDot(
  xiiArrayPtr<const xiiRGCompiledPass>    compiledPasses,
  xiiArrayPtr<const xiiRGBarrierDesc>     barriers,
  xiiArrayPtr<const xiiRGMergeGroup>      mergeGroups,
  xiiArrayPtr<const xiiRGQueueSubmission> queueSubmissions,
  xiiStringBuilder&                       out_sDot)
{
  if (compiledPasses.IsEmpty())
    return XII_FAILURE;

  static constexpr const char* QueueColors[3] = {"#1f6bae", "#ae5b1f", "#1fae5b"};
  static constexpr const char* QueueNames[3]  = {"Graphics", "AsyncCompute", "AsyncTransfer"};

  out_sDot.Clear();
  out_sDot.Append("digraph RenderGraph {\n");
  out_sDot.Append("  graph [fontname=\"Helvetica\" bgcolor=\"#1a1a1a\" fontcolor=\"white\"];\n");
  out_sDot.Append("  node  [fontname=\"Helvetica\" fontsize=10 style=filled shape=box];\n");
  out_sDot.Append("  edge  [fontname=\"Helvetica\" fontsize=8 color=\"#888888\"];\n\n");

  // ── Queue cluster sub-graphs ──────────────────────────────────────────────
  for (xiiUInt32 qi = 0U; qi < queueSubmissions.GetCount(); ++qi)
  {
    const xiiRGQueueSubmission& sub = queueSubmissions[qi];
    const xiiUInt32 uiQueueIdx = sub.m_uiQueueIndex < 3U ? sub.m_uiQueueIndex : 0U;

    out_sDot.AppendFormat("  subgraph cluster_q{0} {{\n", qi);
    out_sDot.AppendFormat("    label=\"Submission {0} — {1}\";\n", qi, QueueNames[uiQueueIdx]);
    out_sDot.AppendFormat("    style=filled; color=\"{0}22\"; fontcolor=\"{0}\";\n", QueueColors[uiQueueIdx]);

    // Merge group clusters inside this queue cluster.
    xiiHashTable<xiiUInt32, xiiDynamicArray<xiiUInt32>> mergeGroupPasses; // groupIdx → sortedPos list
    for (xiiUInt32 sortedPos : sub.m_PassOrder)
    {
      const xiiRGCompiledPass& cp = compiledPasses[sortedPos];
      if (cp.m_uiMergeGroupIndex != xiiInvalidIndex)
        mergeGroupPasses[cp.m_uiMergeGroupIndex].PushBack(sortedPos);
    }

    for (auto it = mergeGroupPasses.GetIterator(); it.IsValid(); ++it)
    {
      out_sDot.AppendFormat("    subgraph cluster_mg{0} {{\n", it.Key());
      out_sDot.Append("      label=\"MergeGroup\"; style=dashed; color=\"#ffaa00\";\n");
      for (xiiUInt32 pos : it.Value())
      {
        const xiiRGCompiledPass& cp = compiledPasses[pos];
        const char* fillColor       = cp.m_bIsCulled ? "#444444" : QueueColors[uiQueueIdx];
        const char* textColor       = cp.m_bIsCulled ? "#888888" : "white";
        out_sDot.AppendFormat("      p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n",
          cp.m_uiPassIndex, cp.m_sName.GetView(),
          fillColor, textColor,
          cp.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
      }
      out_sDot.Append("    }\n");
    }

    // Passes not in any merge group.
    for (xiiUInt32 sortedPos : sub.m_PassOrder)
    {
      const xiiRGCompiledPass& cp = compiledPasses[sortedPos];
      if (cp.m_uiMergeGroupIndex != xiiInvalidIndex) continue;
      const char* fillColor = cp.m_bIsCulled ? "#444444" : QueueColors[uiQueueIdx];
      const char* textColor = cp.m_bIsCulled ? "#888888" : "white";
      out_sDot.AppendFormat("    p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n",
        cp.m_uiPassIndex, cp.m_sName.GetView(),
        fillColor, textColor,
        cp.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
    }

    out_sDot.Append("  }\n\n");
  }

  // ── Dependency edges ──────────────────────────────────────────────────────
  for (const xiiRGCompiledPass& cp : compiledPasses)
  {
    for (xiiUInt32 depIdx : cp.m_DependencyPassIndices)
    {
      out_sDot.AppendFormat("  p{0} -> p{1} [color=\"#4488cc\"];\n",
        compiledPasses[depIdx].m_uiPassIndex, cp.m_uiPassIndex);
    }
  }

  // ── Barrier annotations ───────────────────────────────────────────────────
  for (const xiiRGCompiledPass& cp : compiledPasses)
  {
    if (cp.m_bIsCulled) continue;

    for (xiiUInt32 bIdx : cp.m_PreBarrierIndices)
    {
      const xiiRGBarrierDesc& b = barriers[bIdx];
      const char* edgeLabel = (b.m_TransitionType == xiiGALStateTransitionType::End)
        ? "SplitEnd" : "Immediate";
      const char* edgeColor = (b.m_TransitionType == xiiGALStateTransitionType::End)
        ? "#ffaa00" : "#cc4444";
      (void)edgeLabel; (void)edgeColor;
      // Barrier annotations are embedded in node tooltips in DOT; edges come from dependency pass links.
    }
  }

  out_sDot.Append("}\n");
  return XII_SUCCESS;
}
