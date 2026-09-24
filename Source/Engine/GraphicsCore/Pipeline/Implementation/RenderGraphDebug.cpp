/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

// static
xiiResult xiiRenderGraphDebug::DumpToDot(xiiArrayPtr<const xiiRenderGraphCompiledPass> pCompiledPasses, xiiArrayPtr<const xiiRenderGraphBarrierDescription> pBarriers, xiiArrayPtr<const xiiRenderGraphMergeGroup> pMergeGroups, xiiArrayPtr<const xiiRenderGraphQueueSubmission> pQueueSubmissions, xiiArrayPtr<const xiiRenderGraphResourceDescription> pResources, xiiArrayPtr<const xiiRenderGraphResourceVersionDescription> pVersions, xiiStringBuilder& out_sDot)
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
    const xiiRenderGraphQueueSubmission& submission   = pQueueSubmissions[uiQueueSubmissionIndex];
    const xiiUInt32                      uiQueueIndex = submission.m_uiQueueIndex < 3U ? submission.m_uiQueueIndex : 0U;

    out_sDot.AppendFormat("  submissiongraph cluster_q{0} {{\n", uiQueueSubmissionIndex);
    out_sDot.AppendFormat("    label=\"Submission {0} - {1}\";\n", uiQueueSubmissionIndex, QueueNames[uiQueueIndex]);
    out_sDot.AppendFormat("    style=filled; color=\"{0}22\"; fontcolor=\"{0}\";\n", QueueColors[uiQueueIndex]);

    // Merge group clusters inside this queue cluster.
    xiiHashTable<xiiUInt32, xiiDynamicArray<xiiUInt32>> mergeGroupPasses; // GroupIndex -> Sorted pass list.
    for (xiiUInt32 uiSortedPassOrder : submission.m_PassOrder)
    {
      const xiiRenderGraphCompiledPass& compiledPass = pCompiledPasses[uiSortedPassOrder];

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
        const xiiRenderGraphCompiledPass& compiledPass = pCompiledPasses[uiPassPosition];
        const char*                       szFillColour = compiledPass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
        const char*                       szTextColour = compiledPass.m_bIsCulled ? "#888888" : "white";

        out_sDot.AppendFormat("      p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n", compiledPass.m_uiPassIndex, compiledPass.m_sName.GetView(), szFillColour, szTextColour, compiledPass.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
      }
      out_sDot.Append("    }\n");
    }

    // Passes not in any merge group.
    for (xiiUInt32 uiSortedPassOrder : submission.m_PassOrder)
    {
      const xiiRenderGraphCompiledPass& compiledPass = pCompiledPasses[uiSortedPassOrder];

      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
        continue;

      const char* szFillColour = compiledPass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
      const char* szTextColour = compiledPass.m_bIsCulled ? "#888888" : "white";

      out_sDot.AppendFormat("    p{0} [label=\"{1}\" fillcolor=\"{2}\" fontcolor=\"{3}\"{4}];\n", compiledPass.m_uiPassIndex, compiledPass.m_sName.GetView(), szFillColour, szTextColour, compiledPass.m_bIsCulled ? " style=\"filled,dashed\"" : " style=filled");
    }

    out_sDot.Append("  }\n\n");
  }

  // Dependency edges.
  for (const xiiRenderGraphCompiledPass& compiledPass : pCompiledPasses)
  {
    for (xiiUInt32 depIdx : compiledPass.m_DependencyPassIndices)
    {
      out_sDot.AppendFormat("  p{0} -> p{1} [color=\"#4488cc\"];\n", depIdx, compiledPass.m_uiPassIndex);
    }
  }

  // Barrier annotations.
  for (const xiiRenderGraphCompiledPass& compiledPass : pCompiledPasses)
  {
    if (compiledPass.m_bIsCulled)
      continue;

    for (xiiUInt32 uiBarrierIndex : compiledPass.m_PreBarrierIndices)
    {
      const xiiRenderGraphBarrierDescription& barrier = pBarriers[uiBarrierIndex];
      // const char*            szEdgeLabel  = (barrier.m_TransitionType == xiiGALStateTransitionType::End) ? "SplitEnd" : "Immediate";
      // const char*            szEdgeColour = (barrier.m_TransitionType == xiiGALStateTransitionType::End) ? "#ffaa00" : "#cc4444";

      // Barrier annotations are embedded in node tooltips in DOT, edges come from dependency pass links.
    }
  }

  out_sDot.Append("\n  subgraph cluster_resources {\n    label=\"Resources / Lifetimes / Aliasing\"; color=\"#777777\";\n");
  for (xiiUInt32 i = 0U; i < pResources.GetCount(); ++i)
  {
    const xiiRenderGraphResourceDescription& resource = pResources[i];
    out_sDot.AppendFormat("    r{0} [shape=note fillcolor=\"#303030\" fontcolor=\"white\" label=\"{1}\\nlife [{2},{3}]\\nalias {4}\"];\n", i, resource.m_sName.GetView(), resource.m_uiFirstUsePassIndex, resource.m_uiLastUsePassIndex, resource.m_uiAliasGroup);
  }
  out_sDot.Append("  }\n");

  for (const xiiRenderGraphResourceVersionDescription& version : pVersions)
  {
    xiiUInt32 uiResourceIndex = xiiInvalidIndex;
    for (xiiUInt32 i = 0U; i < pResources.GetCount(); ++i)
    {
      if (pResources[i].m_Id == version.m_ResourceId)
      {
        uiResourceIndex = i;
        break;
      }
    }
    if (uiResourceIndex == xiiInvalidIndex)
      continue;

    for (const xiiRenderGraphCompiledPass& pass : pCompiledPasses)
    {
      if (pass.m_Id == version.m_ProducerPassId)
      {
        out_sDot.AppendFormat("  p{0} -> r{1} [style=dashed color=\"#aa66cc\" label=\"v{2}\"];\n", pass.m_uiPassIndex, uiResourceIndex, version.m_uiVersion);
        break;
      }
    }
  }

  out_sDot.Append("}\n");
  return XII_SUCCESS;
}
