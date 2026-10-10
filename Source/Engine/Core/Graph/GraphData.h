/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/Graph.h>
#include <Core/Graph/GraphMappings.h>
#include <Foundation/Containers/HashTable.h>

/// Typed, owning property maps attached to an existing graph. Payloads may be values, pointers or engine smart pointers.
/// Missing properties are allowed. Removed elements and topology replacement automatically destroy their properties.
/// The graph must outlive this object. Property pointers are transient across changes to their property map.
/// No virtual node base, void pointers, reflection requirements or per-element allocations are imposed.
template <typename VertexData, typename EdgeData>
class xiiGraphData
{
public:
  explicit xiiGraphData(const xiiGraph& graph, xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Graph(graph), m_VertexData(pAllocator), m_EdgeData(pAllocator)
  {
    graph.GetEvents().AddEventHandler(xiiMakeDelegate(&xiiGraphData::OnGraphEvent, this), m_Unsubscriber);
  }
  xiiGraphData(const xiiGraphData&)   = delete;
  void operator=(const xiiGraphData&) = delete;

  template <typename T>
  xiiResult SetVertexData(xiiGraphVertexHandle vertex, T&& data)
  {
    if (!m_Graph.IsValid(vertex))
      return XII_FAILURE;
    m_VertexData.Insert(vertex, std::forward<T>(data));
    return XII_SUCCESS;
  }
  template <typename T>
  xiiResult SetEdgeData(xiiGraphEdgeHandle edge, T&& data)
  {
    if (!m_Graph.IsValid(edge))
      return XII_FAILURE;
    m_EdgeData.Insert(edge, std::forward<T>(data));
    return XII_SUCCESS;
  }
  VertexData* GetVertexData(xiiGraphVertexHandle vertex)
  {
    VertexData* data = nullptr;
    if (m_Graph.IsValid(vertex))
      m_VertexData.TryGetValue(vertex, data);
    return data;
  }
  const VertexData* GetVertexData(xiiGraphVertexHandle vertex) const
  {
    const VertexData* data = nullptr;
    if (m_Graph.IsValid(vertex))
      m_VertexData.TryGetValue(vertex, data);
    return data;
  }
  EdgeData* GetEdgeData(xiiGraphEdgeHandle edge)
  {
    EdgeData* data = nullptr;
    if (m_Graph.IsValid(edge))
      m_EdgeData.TryGetValue(edge, data);
    return data;
  }
  const EdgeData* GetEdgeData(xiiGraphEdgeHandle edge) const
  {
    const EdgeData* data = nullptr;
    if (m_Graph.IsValid(edge))
      m_EdgeData.TryGetValue(edge, data);
    return data;
  }
  void Compact()
  {
    m_VertexData.Compact();
    m_EdgeData.Compact();
  }

private:
  void OnGraphEvent(const xiiGraphEvent& event)
  {
    if (event.m_Type == xiiGraphEvent::Type::VertexRemoved)
      m_VertexData.Remove(event.m_hVertex);
    else if (event.m_Type == xiiGraphEvent::Type::EdgeRemoved)
      m_EdgeData.Remove(event.m_hEdge);
    else if (event.m_Type == xiiGraphEvent::Type::BeforeReset)
    {
      if (!event.m_pMapping)
      {
        m_VertexData.Clear();
        m_EdgeData.Clear();
        return;
      }
      xiiHashTable<xiiGraphVertexHandle, VertexData> vertices(m_VertexData.GetAllocator());
      xiiHashTable<xiiGraphEdgeHandle, EdgeData>     edges(m_EdgeData.GetAllocator());
      for (auto it = m_VertexData.GetIterator(); it.IsValid(); ++it)
      {
        xiiGraphVertexHandle remapped;
        if (event.m_pMapping->m_Vertices.TryGetValue(it.Key(), remapped))
          vertices.Insert(remapped, std::move(it.Value()));
      }
      for (auto it = m_EdgeData.GetIterator(); it.IsValid(); ++it)
      {
        xiiGraphEdgeHandle remapped;
        if (event.m_pMapping->m_Edges.TryGetValue(it.Key(), remapped))
          edges.Insert(remapped, std::move(it.Value()));
      }
      m_VertexData = std::move(vertices);
      m_EdgeData   = std::move(edges);
    }
  }
  const xiiGraph&                                m_Graph;
  xiiHashTable<xiiGraphVertexHandle, VertexData> m_VertexData;
  xiiHashTable<xiiGraphEdgeHandle, EdgeData>     m_EdgeData;
  typename xiiGraph::Event::Unsubscriber         m_Unsubscriber;
};

/// Optional graph-owned typed data. Use the plain graph for topology-only consumers.
template <typename VertexData, typename EdgeData>
struct xiiGraphWithData
{
  explicit xiiGraphWithData(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Graph(pAllocator), m_Data(m_Graph, pAllocator) {}
  xiiGraph                           m_Graph;
  xiiGraphData<VertexData, EdgeData> m_Data;
};
