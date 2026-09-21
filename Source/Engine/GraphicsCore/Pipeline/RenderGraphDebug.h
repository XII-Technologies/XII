/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/ArrayPtr.h>

struct xiiRGCompiledPass;
struct xiiRGBarrierDescription;
struct xiiRGMergeGroup;
struct xiiRGQueueSubmission;

/// Static utility class for render graph debugging and visualization.
///
/// Provides methods to serialize a compiled render graph into human-readable formats suitable for offline visualization with Graphviz or similar tools.
class XII_GRAPHICSCORE_DLL xiiRenderGraphDebug
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphDebug);

public:
  /// Generates a DOT-language description of the compiled render graph.
  ///
  /// The output contains:
  ///  - One node per compiled (non-culled) pass, labeled with the pass name and queue index.
  ///  - Directed edges representing data-flow dependencies (resource reads/writes).
  ///  - Cluster sub-graphs for each merge group (passes that share a native render pass).
  ///  - Cluster sub-graphs by queue (Graphics / AsyncCompute / AsyncTransfer).
  ///  - Edge annotations for barrier type (Immediate / SplitBegin / SplitEnd / UAV).
  ///
  /// \param compiledPasses  - All compiled passes returned by xiiRenderGraph::Compile().
  /// \param barriers        - All barriers from xiiRenderGraph::Compile().
  /// \param mergeGroups     - Merge groups from xiiRenderGraph::Compile().
  /// \param queueSubmission - Queue submission ordering from xiiRenderGraph::Compile().
  /// \param out_sDot        - Receives the DOT text on success.
  [[nodiscard]] static xiiResult DumpToDot(xiiArrayPtr<const xiiRGCompiledPass> pCompiledPasses, xiiArrayPtr<const xiiRGBarrierDescription> pBarriers, xiiArrayPtr<const xiiRGMergeGroup> pMergeGroups, xiiArrayPtr<const xiiRGQueueSubmission> pQueueSubmissions, xiiStringBuilder& out_sDot);
};
