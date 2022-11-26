#pragma once

#include <GameEngine/AI/PointOfInterestGraph.h>

template <typename POINTTYPE>
void xiiPointOfInterestGraph<POINTTYPE>::Initialize(const xiiVec3& center, const xiiVec3& halfExtents, float cellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(center, halfExtents, cellSize);
}

template <typename POINTTYPE>
POINTTYPE& xiiPointOfInterestGraph<POINTTYPE>::AddPoint(const xiiVec3& position)
{
  const xiiUInt32 id = m_Points.GetCount();
  auto&           pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(position, xiiVec3::ZeroVector(), 0, id, nullptr, true).IgnoreResult();

  return pt;
}

template <typename POINTTYPE>
void xiiPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const xiiVec3& position, float radius, xiiDynamicArray<xiiUInt32>& out_Points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    xiiDynamicArray<xiiUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_Points;

  auto cb = [](void* pPassThrough, xiiDynamicTreeObjectConst Object) -> bool {
    auto pData = static_cast<Data*>(pPassThrough);

    const xiiUInt32 id = (xiiUInt32)Object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(position, radius, cb, &data);
}
