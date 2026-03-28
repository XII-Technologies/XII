#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

// static
xiiResult xiiRenderGraphDebug::DumpToDot(xiiArrayPtr<const xiiRGCompiledPass> pCompiledPasses, xiiArrayPtr<const xiiRGBarrierDesc> pBarriers, xiiArrayPtr<const xiiRGMergeGroup> pMergeGroups, xiiArrayPtr<const xiiRGQueueSubmission> pQueueSubmissions, xiiStringBuilder& out_sDot)
{
  if (pCompiledPasses.IsEmpty())
    return XII_FAILURE;

  static constexpr const char* QueueColors[3] = {"#1f6bae", "#ae5b1f", "#1fae5b"};
  static constexpr const char* QueueNames[3]  = {"Graphics", "AsyncCompute", "AsyncTransfer"};

  out_sDot.Clear();
  out_sDot.Append("digraph RenderGraph {\n");
  out_sDot.Append("  graph [fontname=\"Helvetica\" bgcolor=\"#1a1a1a\" fontcolor=\"white\"];\n");
  out_sDot.Append("  node  [fontname=\"Helvetica\" fontsize=10 style=filled shape=box];\n");
  out_sDot.Append("  edge  [fontname=\"Helvetica\" fontsize=8 color=\"#888888\"];\n\n");

  // Queue cluster sub-graphs
  for (xiiUInt32 uiQueueSubmissionIndex = 0U; uiQueueSubmissionIndex < pQueueSubmissions.GetCount(); ++uiQueueSubmissionIndex)
  {
    const xiiRGQueueSubmission& submission   = pQueueSubmissions[uiQueueSubmissionIndex];
    const xiiUInt32             uiQueueIndex = submission.m_uiQueueIndex < 3U ? submission.m_uiQueueIndex : 0U;

    out_sDot.AppendFormat("  submissiongraph cluster_q{0} {{\n", uiQueueSubmissionIndex);
    out_sDot.AppendFormat("    label=\"Submission {0} — {1}\";\n", uiQueueSubmissionIndex, QueueNames[uiQueueIndex]);
    out_sDot.AppendFormat("    style=filled; color=\"{0}22\"; fontcolor=\"{0}\";\n", QueueColors[uiQueueIndex]);

    // Merge group clusters inside this queue cluster.
    xiiHashTable<xiiUInt32, xiiDynamicArray<xiiUInt32>> mergeGroupPasses; // GroupIndex -> Sorted pass list.
    for (xiiUInt32 uiSortedPassOrder : submission.m_PassOrder)
    {
      const xiiRGCompiledPass& compiledPass = pCompiledPasses[uiSortedPassOrder];

      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        mergeGroupPasses[compiledPass.m_uiMergeGroupIndex].PushBack(uiSortedPassOrder);
      }
    }

    for (auto it = mergeGroupPasses.GetIterator(); it.IsValid(); ++it)
    {
      out_sDot.AppendFormat("    subgraph cluster_mg{0} {{\n", it.Key());
      out_sDot.Append("      label=\"MergeGroup\"; style=dashed; color=\"#ffaa00\";\n");

      for (xiiUInt32 uiPassPosition : it.Value())
      {
        const xiiRGCompiledPass& compiledPass = pCompiledPasses[uiPassPosition];
        const char*              szFillColour = compiledPass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
        const char*              szTextColour = compiledPass.m_bIsCulled ? "#888888" : "white";

        out_sDot.AppendFormat("      p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n", compiledPass.m_uiPassIndex, compiledPass.m_sName.GetView(), szFillColour, szTextColour, compiledPass.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
      }
      out_sDot.Append("    }\n");
    }

    // Passes not in any merge group.
    for (xiiUInt32 uiSortedPassOrder : submission.m_PassOrder)
    {
      const xiiRGCompiledPass& compiledPass = pCompiledPasses[uiSortedPassOrder];

      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
        continue;

      const char* szFillColour = compiledPass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
      const char* szTextColour = compiledPass.m_bIsCulled ? "#888888" : "white";

      out_sDot.AppendFormat("    p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n", compiledPass.m_uiPassIndex, compiledPass.m_sName.GetView(), szFillColour, szTextColour, compiledPass.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
    }

    out_sDot.Append("  }\n\n");
  }

  // Dependency edges.
  for (const xiiRGCompiledPass& compiledPass : pCompiledPasses)
  {
    for (xiiUInt32 depIdx : compiledPass.m_DependencyPassIndices)
    {
      out_sDot.AppendFormat("  p{0} -> p{1} [color=\"#4488cc\"];\n", pCompiledPasses[depIdx].m_uiPassIndex, compiledPass.m_uiPassIndex);
    }
  }

  // Barrier annotations.
  for (const xiiRGCompiledPass& compiledPass : pCompiledPasses)
  {
    if (compiledPass.m_bIsCulled)
      continue;

    for (xiiUInt32 uiBarrierIndex : compiledPass.m_PreBarrierIndices)
    {
      const xiiRGBarrierDesc& barrier      = pBarriers[uiBarrierIndex];
      // const char*          szEdgeLabel  = (barrier.m_TransitionType == xiiGALStateTransitionType::End) ? "SplitEnd" : "Immediate";
      // const char*          szEdgeColour = (barrier.m_TransitionType == xiiGALStateTransitionType::End) ? "#ffaa00" : "#cc4444";

      // Barrier annotations are embedded in node tooltips in DOT, edges come from dependency pass links.
    }
  }

  out_sDot.Append("}\n");
  return XII_SUCCESS;
}
