/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/Graph.h>
#include <Foundation/Types/Delegate.h>

/// Reusable O(V) scratch storage. No topology is retained between calls. One workspace per concurrent invocation.
/// The allocator must outlive the workspace. Callbacks must not reenter an algorithm using the same workspace.
class XII_CORE_DLL xiiGraphAlgorithmWorkspace
{
public:
  explicit xiiGraphAlgorithmWorkspace(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());

private:
  friend class xiiGraphAlgorithms;
  struct Frame
  {
    xiiUInt32              m_uiVertex;
    xiiGraph::EdgeIterator m_Edges;
  };
  void                       Reset(xiiUInt32 vertexCount);
  xiiDynamicArray<xiiUInt32> m_State;
  xiiDynamicArray<xiiUInt32> m_Work;
  xiiDynamicArray<xiiUInt32> m_Order;
  xiiDynamicArray<Frame>     m_Frames;
};

/// Flat component output; offsets contains a leading zero and one past-the-end offset for each component.
/// Component and vertex orders are deterministic for a fixed graph, but carry no semantic ordering guarantee.
struct XII_CORE_DLL xiiGraphComponents
{
  explicit xiiGraphComponents(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());
  xiiUInt32                               GetCount() const { return m_Offsets.IsEmpty() ? 0 : m_Offsets.GetCount() - 1; }
  xiiArrayPtr<const xiiGraphVertexHandle> GetComponent(xiiUInt32 index) const;

  xiiDynamicArray<xiiGraphVertexHandle> m_Vertices;
  xiiDynamicArray<xiiUInt32>            m_Offsets;
};

/// Iterative algorithms over xiiGraph's public connectivity interface. No recursion or permanent graph bookkeeping.
/// All operations take O(V + E) time and O(V) auxiliary space (including initialization for single-source queries).
/// Outputs are replaced, retaining their allocations. Pass a workspace to reuse temporary allocations too.
/// Without one, scratch uses the default allocator. The graph must remain unchanged throughout each call.
class XII_CORE_DLL xiiGraphAlgorithms
{
public:
  /// Return false from a visitor to stop successfully. Each reachable vertex is visited at most once.
  using Visitor = xiiDelegate<bool(xiiGraphVertexHandle)>;

  /// Visit start and its reachable successors, following outgoing edges. Other components are not visited.
  /// Breadth first visits by nondecreasing distance; depth first visits in preorder. Adjacency is newest first.
  /// An invalid/foreign start or empty visitor returns XII_FAILURE without invoking the visitor.
  static xiiResult BreadthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);
  static xiiResult DepthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);

  /// Zero-length paths are reachable. Invalid/foreign endpoints are never reachable.
  static bool IsReachable(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);

  /// Kahn's algorithm. Includes isolated vertices and disconnected components. Source precedes target for every edge.
  /// Returns XII_FAILURE and clears output on any cycle (including self-loops). Empty graphs succeed.
  /// Ready vertices use FIFO order seeded by GetVertices(); ties are otherwise unspecified.
  static xiiResult TopologicalSort(const xiiGraph& graph, xiiDynamicArray<xiiGraphVertexHandle>& out_order, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);
  static bool      HasCycle(const xiiGraph& graph, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);

  /// Iterative Kosaraju: vertices in a component are mutually reachable through directed edges.
  static void StronglyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);
  /// Components when edge direction is ignored. An isolated vertex is a singleton component.
  static void WeaklyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr);

private:
  static void      WalkDepthFirst(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace& scratch);
  static xiiResult SortTopologically(const xiiGraph& graph, xiiGraphAlgorithmWorkspace& scratch);
};
