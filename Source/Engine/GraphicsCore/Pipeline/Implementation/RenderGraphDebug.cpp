/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

xiiResult xiiRenderGraphDebug::DumpToDot(xiiArrayPtr<const xiiRenderGraphCompiledPass> pCompiledPasses, xiiArrayPtr<const xiiRenderGraphBarrierDescription> pBarriers, xiiArrayPtr<const xiiRenderGraphMergeGroup> pMergeGroups, xiiArrayPtr<const xiiRenderGraphQueueSubmission> pQueueSubmissions, xiiArrayPtr<const xiiRenderGraphResourceDescription> pResources, xiiArrayPtr<const xiiRenderGraphResourceVersionDescription> pVersions, xiiStringBuilder& out_sDot)
{
  if (pCompiledPasses.IsEmpty())
    return XII_SUCCESS;

  static constexpr const char* QueueColors[3] = {"#1f6bae", "#ae5b1f", "#1fae5b"};
  static constexpr const char* QueueNames[3]  = {"Graphics", "AsyncCompute", "AsyncTransfer"};

  out_sDot.Clear();
  out_sDot.Append(
    "digraph RenderGraph {\n"
    "  graph [fontname=\"Helvetica\" bgcolor=\"#1a1a1a\" fontcolor=\"white\" rankdir=LR];\n"
    "  node  [fontname=\"Helvetica\" fontsize=10 style=filled shape=box];\n"
    "  edge  [fontname=\"Helvetica\" fontsize=8 color=\"#888888\"];\n\n");

  // ------------------------------------------------------------
  // Queue clusters
  // ------------------------------------------------------------
  for (xiiUInt32 uiQueueSubmissionIndex = 0; uiQueueSubmissionIndex < pQueueSubmissions.GetCount(); ++uiQueueSubmissionIndex)
  {
    const xiiRenderGraphQueueSubmission& submission   = pQueueSubmissions[uiQueueSubmissionIndex];
    const xiiUInt32                      uiQueueIndex = submission.m_uiQueueIndex < 3 ? submission.m_uiQueueIndex : 0;

    out_sDot.AppendFormat("  subgraph cluster_queue_{} {\n", uiQueueSubmissionIndex);
    out_sDot.AppendFormat("    label=\"Submission {} - {}\";\n", uiQueueSubmissionIndex, QueueNames[uiQueueIndex]);
    out_sDot.AppendFormat("    style=filled; color=\"{}\"; fontcolor=\"{}\";\n", QueueColors[uiQueueIndex], QueueColors[uiQueueIndex]);

    // ------------------------------------------------------------
    // Merge groups inside queue
    // ------------------------------------------------------------
    xiiHashTable<xiiUInt32, xiiDynamicArray<xiiUInt32>> mergeGroups;
    for (xiiUInt32 uiPassOrder : submission.m_PassOrder)
    {
      const xiiRenderGraphCompiledPass& pass = pCompiledPasses[uiPassOrder];

      if (pass.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        mergeGroups[pass.m_uiMergeGroupIndex].PushBack(uiPassOrder);
      }
    }

    for (auto it = mergeGroups.GetIterator(); it.IsValid(); ++it)
    {
      out_sDot.AppendFormat("    subgraph cluster_mg_{}_{} {\n", uiQueueSubmissionIndex, it.Key());
      out_sDot.Append("      label=\"MergeGroup\"; style=dashed; color=\"#ffaa00\";\n");

      for (xiiUInt32 uiPassOrder : it.Value())
      {
        const xiiRenderGraphCompiledPass& pass   = pCompiledPasses[uiPassOrder];
        const char*                       szFill = pass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
        const char*                       szText = pass.m_bIsCulled ? "#888888" : "white";

        out_sDot.AppendFormat(
          "      q{}_p{} [label=\"{}\" fillcolor=\"{}\" fontcolor=\"{}\" style=\"{}\"];\n",
          uiQueueSubmissionIndex, pass.m_uiPassIndex, pass.m_sName.GetView(), szFill, szText, pass.m_bIsCulled ? "filled,dashed" : "filled");
      }

      out_sDot.Append("    }\n");
    }

    // ------------------------------------------------------------
    // Passes not in merge groups
    // ------------------------------------------------------------
    for (xiiUInt32 uiPassOrder : submission.m_PassOrder)
    {
      const xiiRenderGraphCompiledPass& pass = pCompiledPasses[uiPassOrder];
      if (pass.m_uiMergeGroupIndex != xiiInvalidIndex)
        continue;

      const char* szFill = pass.m_bIsCulled ? "#444444" : QueueColors[uiQueueIndex];
      const char* szText = pass.m_bIsCulled ? "#888888" : "white";

      out_sDot.AppendFormat(
        "    q{}_p{} [label=\"{}\" fillcolor=\"{}\" fontcolor=\"{}\" style=\"{}\"];\n",
        uiQueueSubmissionIndex, pass.m_uiPassIndex, pass.m_sName.GetView(), szFill, szText, pass.m_bIsCulled ? "filled,dashed" : "filled");
    }

    out_sDot.Append("  }\n\n");
  }

  // ------------------------------------------------------------
  // Dependency edges
  // ------------------------------------------------------------
  for (const xiiRenderGraphCompiledPass& pass : pCompiledPasses)
  {
    for (xiiUInt32 uiDependencyPassIndex : pass.m_DependencyPassIndices)
    {
      // All passes are in queue 0 for dependency edges.
      out_sDot.AppendFormat("  q0_p{} -> q0_p{} [color=\"#4488cc\"];\n", uiDependencyPassIndex, pass.m_uiPassIndex);
    }
  }

  // ------------------------------------------------------------
  // Resource cluster (record shapes)
  // ------------------------------------------------------------
  out_sDot.Append(
    "\n  subgraph cluster_resources {\n"
    "    label=\"Resources / Lifetimes / Aliasing\";\n"
    "    style=filled; color=\"#555555\";\n"
    "    node [shape=record fillcolor=\"#303030\" fontcolor=\"white\"];\n");

  for (xiiUInt32 i = 0; i < pResources.GetCount(); ++i)
  {
    const xiiRenderGraphResourceDescription& description = pResources[i];

    out_sDot.AppendFormat(
      "    r{} [label=\"{{{} | life [{},{}] | alias {}}}\"];\n",
      i, description.m_sName.GetView(), description.m_uiFirstUsePassIndex, description.m_uiLastUsePassIndex, description.m_uiAliasGroup);
  }

  out_sDot.Append("  }\n");

  // ------------------------------------------------------------
  // Resource version edges
  // ------------------------------------------------------------
  for (const xiiRenderGraphResourceVersionDescription& versionDescription : pVersions)
  {
    xiiUInt32 uiResourceIndex = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < pResources.GetCount(); ++i)
    {
      if (pResources[i].m_Id == versionDescription.m_ResourceId)
      {
        uiResourceIndex = i;
      }
    }

    if (uiResourceIndex == xiiInvalidIndex)
      continue;

    for (const xiiRenderGraphCompiledPass& pass : pCompiledPasses)
    {
      if (pass.m_Id == versionDescription.m_ProducerPassId)
      {
        out_sDot.AppendFormat(
          "  q0_p{} -> r{} [style=dashed color=\"#aa66cc\" label=\"v{}\"];\n",
          pass.m_uiPassIndex, uiResourceIndex, versionDescription.m_uiVersion);
        break;
      }
    }
  }

  out_sDot.Append("}\n");
  return XII_SUCCESS;
}
