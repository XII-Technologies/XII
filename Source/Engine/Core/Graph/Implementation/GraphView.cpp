/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphSerialization.h>
#include <Core/Graph/GraphView.h>

xiiResult xiiGraphView::Select(xiiArrayPtr<const xiiGraphVertexHandle> vertices, Mode mode, xiiArrayPtr<const xiiGraphEdgeHandle> edges)
{
  xiiHashSet<xiiGraphVertexHandle> selectedVertices;
  xiiHashSet<xiiGraphEdgeHandle>   selectedEdges;
  for (auto vertex : vertices)
  {
    if (!m_Graph.IsValid(vertex))
      return XII_FAILURE;
    selectedVertices.Insert(vertex);
  }
  for (auto edge : edges)
  {
    if (!m_Graph.IsValid(edge) || !selectedVertices.Contains(m_Graph.GetSource(edge)) || !selectedVertices.Contains(m_Graph.GetTarget(edge)))
      return XII_FAILURE;
    selectedEdges.Insert(edge);
  }
  m_Vertices = std::move(selectedVertices);
  m_Edges    = std::move(selectedEdges);
  m_Mode     = mode;
  return XII_SUCCESS;
}

bool xiiGraphView::Contains(xiiGraphVertexHandle vertex) const
{
  return m_Graph.IsValid(vertex) && m_Vertices.Contains(vertex);
}

bool xiiGraphView::Contains(xiiGraphEdgeHandle edge) const
{
  return m_Graph.IsValid(edge) && (m_Mode == Mode::Induced || m_Edges.Contains(edge)) && Contains(m_Graph.GetSource(edge)) && Contains(m_Graph.GetTarget(edge));
}

void xiiGraphView::Enumerate(xiiDynamicArray<xiiGraphVertexHandle>& out_vertices, xiiDynamicArray<xiiGraphEdgeHandle>& out_edges) const
{
  out_vertices.Clear();
  out_edges.Clear();
  for (auto vertex : m_Graph.GetVertices())
    if (Contains(vertex))
      out_vertices.PushBack(vertex);
  for (auto edge : m_Graph.GetEdges())
    if (Contains(edge))
      out_edges.PushBack(edge);
}

void xiiGraphView::Refresh()
{
  for (auto it = m_Vertices.GetIterator(); it.IsValid();)
    if (!m_Graph.IsValid(it.Key()))
      it = m_Vertices.Remove(it);
    else
      ++it;
  for (auto it = m_Edges.GetIterator(); it.IsValid();)
    if (!Contains(it.Key()))
      it = m_Edges.Remove(it);
    else
      ++it;
}

xiiResult xiiGraphView::Remap(const xiiGraphHandleMap& mapping)
{
  xiiDynamicArray<xiiGraphVertexHandle> vertices;
  xiiDynamicArray<xiiGraphEdgeHandle>   edges;
  for (auto it = m_Vertices.GetIterator(); it.IsValid(); ++it)
  {
    xiiGraphVertexHandle vertex;
    if (mapping.m_Vertices.TryGetValue(it.Key(), vertex))
      vertices.PushBack(vertex);
  }
  for (auto it = m_Edges.GetIterator(); it.IsValid(); ++it)
  {
    xiiGraphEdgeHandle edge;
    if (mapping.m_Edges.TryGetValue(it.Key(), edge))
      edges.PushBack(edge);
  }
  return Select(vertices, m_Mode, edges);
}

xiiResult xiiGraphView::CopyTo(xiiGraph& destination, xiiGraphHandleMap& out_mapping) const
{
  xiiDynamicArray<xiiGraphVertexHandle> vertices;
  xiiDynamicArray<xiiGraphEdgeHandle>   edges;
  Enumerate(vertices, edges);
  xiiHashTable<xiiGraphVertexHandle, xiiUInt32> indices;
  xiiGraphSnapshot                              snapshot;
  snapshot.m_uiVertexCount = vertices.GetCount();
  for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
    indices.Insert(vertices[i], i);
  for (auto edge : edges)
  {
    snapshot.m_EdgeSources.PushBack(indices[m_Graph.GetSource(edge)]);
    snapshot.m_EdgeTargets.PushBack(indices[m_Graph.GetTarget(edge)]);
  }
  xiiGraphLoadedHandles loaded;
  XII_SUCCEED_OR_RETURN(snapshot.Apply(destination, &loaded));
  xiiGraphHandleMap mapping;
  for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
    mapping.m_Vertices.Insert(vertices[i], loaded.m_Vertices[i]);
  for (xiiUInt32 i = 0; i < edges.GetCount(); ++i)
    mapping.m_Edges.Insert(edges[i], loaded.m_Edges[i]);
  out_mapping = std::move(mapping);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Graph_Implementation_GraphView);
