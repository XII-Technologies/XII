/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Graph/GraphAlgorithms.h>

xiiGraphAlgorithmWorkspace::xiiGraphAlgorithmWorkspace(xiiAllocator* pAllocator) :
  m_State(pAllocator), m_Work(pAllocator), m_Order(pAllocator), m_Frames(pAllocator), m_Distances(pAllocator), m_EdgeValues(pAllocator), m_EdgeFlows(pAllocator), m_PreviousEdges(pAllocator), m_WeightedEdges(pAllocator)
{
}

void xiiGraphAlgorithmWorkspace::Reset(xiiUInt32 vertexCount)
{
  m_State.SetCount(vertexCount);
  for (auto& state : m_State)
    state = 0;
  m_Work.Clear();
  m_Order.Clear();
  m_Frames.Clear();
}

xiiGraphComponents::xiiGraphComponents(xiiAllocator* pAllocator) :
  m_Vertices(pAllocator), m_Offsets(pAllocator)
{
}

xiiArrayPtr<const xiiGraphVertexHandle> xiiGraphComponents::GetComponent(xiiUInt32 index) const
{
  XII_ASSERT_DEV(index < GetCount(), "Invalid component index.");
  return m_Vertices.GetArrayPtr().GetSubArray(m_Offsets[index], m_Offsets[index + 1] - m_Offsets[index]);
}
