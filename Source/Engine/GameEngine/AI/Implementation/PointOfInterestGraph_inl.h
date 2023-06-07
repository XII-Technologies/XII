#pragma once

#include <GameEngine/AI/PointOfInterestGraph.h>

template <typename POINTTYPE>
void xiiPointOfInterestGraph<POINTTYPE>::Initialize(const xiiVec3& vCenter, const xiiVec3& vHalfExtents, float fCellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(center, halfExtents, cellSize);
}

template <typename POINTTYPE>
POINTTYPE& xiiPointOfInterestGraph<POINTTYPE>::AddPoint(const xiiVec3& vPosition)
{
  const xiiUInt32 id = m_Points.GetCount();
  auto&           pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(position, xiiVec3::ZeroVector(), 0, id, nullptr, true).IgnoreResult();

  return pt;
}

template <typename POINTTYPE>
void xiiPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const xiiVec3& vPosition, float fRadius, xiiDynamicArray<xiiUInt32>& out_points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    xiiDynamicArray<xiiUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_Points;

  auto cb = [](void* pPassThrough, xiiDynamicTreeObjectConst object) -> bool {
    auto pData = static_cast<Data*>(pPassThrough);

    const xiiUInt32 id = (xiiUInt32)object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(position, radius, cb, &data);
}
