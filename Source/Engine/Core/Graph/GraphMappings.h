/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/GraphHandles.h>
#include <Foundation/Containers/HashTable.h>

/// Old-to-new runtime identities returned by copying or compaction.
struct xiiGraphHandleMap
{
  explicit xiiGraphHandleMap(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Vertices(pAllocator), m_Edges(pAllocator)
  {
  }

  void Clear()
  {
    m_Vertices.Clear();
    m_Edges.Clear();
  }

  xiiHashTable<xiiGraphVertexHandle, xiiGraphVertexHandle> m_Vertices;
  xiiHashTable<xiiGraphEdgeHandle, xiiGraphEdgeHandle>     m_Edges;
};
