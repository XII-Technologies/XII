/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/GraphHandles.h>
#include <Foundation/Containers/DynamicArray.h>

struct xiiGraphPath
{
  explicit xiiGraphPath(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Vertices(pAllocator), m_Edges(pAllocator)
  {
  }

  void Clear()
  {
    m_Vertices.Clear();
    m_Edges.Clear();
    m_fCost = 0;
  }

  xiiDynamicArray<xiiGraphVertexHandle> m_Vertices;
  xiiDynamicArray<xiiGraphEdgeHandle>   m_Edges;
  double                                m_fCost = 0;
};

/// Flow per graph edge, in the accompanying handle array's order. Excluded edges have zero flow.
struct xiiGraphFlow
{
  explicit xiiGraphFlow(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Edges(pAllocator), m_Flows(pAllocator)
  {
  }

  void Clear()
  {
    m_Edges.Clear();
    m_Flows.Clear();
    m_fValue = 0;
  }

  xiiDynamicArray<xiiGraphEdgeHandle> m_Edges;
  xiiDynamicArray<double>             m_Flows;
  double                              m_fValue = 0;
};

struct xiiGraphSpanningForest
{
  explicit xiiGraphSpanningForest(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Edges(pAllocator)
  {
  }

  void Clear()
  {
    m_Edges.Clear();
    m_fWeight          = 0;
    m_uiComponentCount = 0;
  }

  xiiDynamicArray<xiiGraphEdgeHandle> m_Edges;
  double                              m_fWeight          = 0;
  xiiUInt32                           m_uiComponentCount = 0;
};
