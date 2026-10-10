/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/Graph.h>
#include <Foundation/Containers/IdTable.h>

struct xiiGraph::Storage
{
  struct Vertex
  {
    LocalId   m_FirstIncoming;
    LocalId   m_FirstOutgoing;
    xiiUInt32 m_uiInDegree   = 0;
    xiiUInt32 m_uiOutDegree  = 0;
    xiiUInt32 m_uiDenseIndex = xiiInvalidIndex;
  };

  struct Edge
  {
    LocalId   m_Source;
    LocalId   m_Target;
    LocalId   m_PreviousIncoming;
    LocalId   m_NextIncoming;
    LocalId   m_PreviousOutgoing;
    LocalId   m_NextOutgoing;
    xiiUInt32 m_uiDenseIndex = xiiInvalidIndex;
  };

  explicit Storage(xiiAllocator* pAllocator, xiiUInt64 identity) :
    m_Vertices(pAllocator), m_Edges(pAllocator), m_VertexHandles(pAllocator), m_EdgeHandles(pAllocator), m_uiGraphId(identity)
  {
  }

  xiiIdTable<LocalId, Vertex>           m_Vertices;
  xiiIdTable<LocalId, Edge>             m_Edges;
  xiiDynamicArray<xiiGraphVertexHandle> m_VertexHandles;
  xiiDynamicArray<xiiGraphEdgeHandle>   m_EdgeHandles;
  xiiUInt64                             m_uiGraphId;
};
