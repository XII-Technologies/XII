/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/Graph.h>
#include <Core/Graph/GraphAlgorithmTypes.h>
#include <Core/Graph/GraphOptimizationTypes.h>
#include <Foundation/Types/Delegate.h>

/// Reusable scratch: O(V) for traversal/path algorithms, O(V + E) for flow/spanning algorithms.
/// Contents are reset before use; no algorithm state is stored in the graph. One workspace per invocation.
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
  struct WeightedEdge
  {
    XII_DECLARE_POD_TYPE();
    xiiGraphEdgeHandle m_hEdge;
    double             m_fWeight;
    xiiUInt32          m_uiOrder;
  };
  xiiDynamicArray<double>             m_Distances;
  xiiDynamicArray<double>             m_EdgeValues;
  xiiDynamicArray<double>             m_EdgeFlows;
  xiiDynamicArray<xiiGraphEdgeHandle> m_PreviousEdges;
  xiiDynamicArray<WeightedEdge>       m_WeightedEdges;
};

/// Flat component output; offsets contains a leading zero and one past-the-end offset for each component.
/// Component and vertex orders are deterministic for a fixed graph, but carry no semantic ordering guarantee.
struct XII_CORE_DLL xiiGraphComponents
{
  explicit xiiGraphComponents(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());
  xiiUInt32 GetCount() const { return m_Offsets.IsEmpty() ? 0 : m_Offsets.GetCount() - 1; }
  /// Requires index < GetCount(). The returned view is invalidated by changes to this result.
  xiiArrayPtr<const xiiGraphVertexHandle> GetComponent(xiiUInt32 index) const;

  xiiDynamicArray<xiiGraphVertexHandle> m_Vertices;
  xiiDynamicArray<xiiUInt32>            m_Offsets;
};

/// Iterative algorithms over xiiGraph's public connectivity interface. No recursion or permanent graph bookkeeping.
/// Basic traversal/connectivity operations take O(V + E) time and O(V) auxiliary space; optimizations specify their bounds below.
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
  static xiiResult BreadthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  static xiiResult DepthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});

  /// Zero-length paths are reachable. Invalid/foreign endpoints are never reachable.
  static bool IsReachable(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});

  /// Kahn's algorithm. Includes isolated vertices and disconnected components. Source precedes target for every edge.
  /// Returns XII_FAILURE and clears output on any cycle (including self-loops). Empty graphs succeed.
  /// Ready vertices use FIFO order seeded by GetVertices(); ties are otherwise unspecified.
  static xiiResult TopologicalSort(const xiiGraph& graph, xiiDynamicArray<xiiGraphVertexHandle>& out_order, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  static bool      HasCycle(const xiiGraph& graph, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});

  /// Iterative Kosaraju: vertices in a component are mutually reachable through directed edges.
  static void StronglyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  /// Components when edge direction is ignored. An isolated vertex is a singleton component.
  static void WeaklyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});

  /// BFS shortest directed path, O(V + E), O(V) scratch. Failure clears output; source == target succeeds.
  static xiiResult FindShortestPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphPath& out_path,
                                    xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  /// Indexed-heap Dijkstra, O((V + E) log V), O(V) scratch. Examined weights must be finite and nonnegative.
  static xiiResult FindLowestCostPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeWeight weight,
                                      xiiGraphPath& out_path, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  /// Edmonds-Karp, O(V + E + V E^2), O(V + E) scratch. Capacities must be finite/nonnegative; source and target must differ.
  /// Parallel and opposing edges have independent capacities; incoming edges provide residual reverse arcs.
  static xiiResult MaximumFlow(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeWeight capacity,
                               xiiGraphFlow& out_flow, xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});
  /// Kruskal, ignoring edge direction explicitly. O(E log E + V), O(V + E) scratch. Empty weight means unit weights.
  /// Disconnected/isolated vertices produce a forest; loops are ignored and parallel edges compete independently.
  static xiiResult MinimumSpanningForest(const xiiGraph& graph, xiiGraphSpanningForest& out_forest, xiiGraphEdgeWeight weight = {},
                                         xiiGraphAlgorithmWorkspace* pWorkspace = nullptr, xiiGraphEdgeFilter filter = {});

private:
  static xiiResult BuildPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace& scratch, xiiGraphPath& out_path);
  static void      WalkDepthFirst(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace& scratch, xiiGraphEdgeFilter filter);
  static xiiResult SortTopologically(const xiiGraph& graph, xiiGraphAlgorithmWorkspace& scratch, xiiGraphEdgeFilter filter);
};
