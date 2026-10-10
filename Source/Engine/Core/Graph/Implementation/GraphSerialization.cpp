/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphSerialization.h>

xiiGraphSnapshot::xiiGraphSnapshot(xiiAllocator* pAllocator) : m_EdgeSources(pAllocator), m_EdgeTargets(pAllocator) {}

void xiiGraphSnapshot::Capture(const xiiGraph& graph)
{
  m_uiVertexCount = graph.GetVertexCount();
  m_EdgeSources.Clear();
  m_EdgeTargets.Clear();
  for (auto edge : graph.GetEdges())
  {
    m_EdgeSources.PushBack(graph.GetVertexIndex(graph.GetSource(edge)));
    m_EdgeTargets.PushBack(graph.GetVertexIndex(graph.GetTarget(edge)));
  }
}

xiiResult xiiGraphSnapshot::Validate() const
{
  if (m_uiVertexCount > xiiGraph::MaxElementCount || m_EdgeSources.GetCount() != m_EdgeTargets.GetCount() || m_EdgeSources.GetCount() > xiiGraph::MaxElementCount)
    return XII_FAILURE;
  for (xiiUInt32 i = 0; i < m_EdgeSources.GetCount(); ++i)
  {
    if (m_EdgeSources[i] >= m_uiVertexCount || m_EdgeTargets[i] >= m_uiVertexCount)
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGraphSnapshot::Apply(xiiGraph& graph, xiiGraphLoadedHandles* out_pHandles) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  if (m_uiVertexCount > graph.GetVertexLimit() || m_EdgeSources.GetCount() > graph.GetEdgeLimit())
    return XII_FAILURE;
  xiiGraph replacement(graph.GetAllocator());
  XII_SUCCEED_OR_RETURN(replacement.Reserve(m_uiVertexCount, m_EdgeSources.GetCount()));
  xiiGraphLoadedHandles handles(graph.GetAllocator());
  for (xiiUInt32 i = 0; i < m_uiVertexCount; ++i)
  {
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(replacement.TryAddVertex(vertex));
    handles.m_Vertices.PushBack(vertex);
  }
  for (xiiUInt32 i = 0; i < m_EdgeSources.GetCount(); ++i)
  {
    xiiGraphEdgeHandle edge;
    XII_SUCCEED_OR_RETURN(replacement.TryAddEdge(handles.m_Vertices[m_EdgeSources[i]], handles.m_Vertices[m_EdgeTargets[i]], edge));
    handles.m_Edges.PushBack(edge);
  }
  XII_SUCCEED_OR_RETURN(graph.MoveFrom(std::move(replacement)));
  if (out_pHandles)
  {
    out_pHandles->m_Vertices = std::move(handles.m_Vertices);
    out_pHandles->m_Edges    = std::move(handles.m_Edges);
  }
  return XII_SUCCESS;
}

xiiResult xiiGraphSnapshot::Save(xiiStreamWriter& stream) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  const xiiUInt32 version   = 1;
  const xiiUInt32 edgeCount = m_EdgeSources.GetCount();
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&version));
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&m_uiVertexCount));
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&edgeCount));
  for (xiiUInt32 i = 0; i < edgeCount; ++i)
  {
    XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&m_EdgeSources[i]));
    XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&m_EdgeTargets[i]));
  }
  return XII_SUCCESS;
}

xiiResult xiiGraphSnapshot::Load(xiiStreamReader& stream, xiiUInt32 vertexLimit, xiiUInt32 edgeLimit)
{
  xiiUInt32 version, vertexCount, edgeCount;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&version));
  if (version != 1)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&vertexCount));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&edgeCount));
  if (vertexCount > vertexLimit || edgeCount > edgeLimit || vertexCount > xiiGraph::MaxElementCount || edgeCount > xiiGraph::MaxElementCount)
    return XII_FAILURE;
  xiiGraphSnapshot loaded(m_EdgeSources.GetAllocator());
  loaded.m_uiVertexCount = vertexCount;
  for (xiiUInt32 i = 0; i < edgeCount; ++i)
  {
    xiiUInt32 source, target;
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&source));
    XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&target));
    if (source >= vertexCount || target >= vertexCount)
      return XII_FAILURE;
    loaded.m_EdgeSources.PushBack(source);
    loaded.m_EdgeTargets.PushBack(target);
  }
  m_uiVertexCount = loaded.m_uiVertexCount;
  m_EdgeSources   = std::move(loaded.m_EdgeSources);
  m_EdgeTargets   = std::move(loaded.m_EdgeTargets);
  return XII_SUCCESS;
}
