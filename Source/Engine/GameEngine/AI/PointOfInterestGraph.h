#pragma once

#include <Foundation/Math/Vec3.h>
#include <GameEngine/GameEngineDLL.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template <typename POINTTYPE>
class xiiPointOfInterestGraph
{
public:
  void Initialize(const xiiVec3& vCenter, const xiiVec3& vHalfExtents, float fCellSize = 1.0f);

  POINTTYPE& AddPoint(const xiiVec3& vPosition);

  void FindPointsOfInterest(const xiiVec3& vPosition, float fRadius, xiiDynamicArray<xiiUInt32>& out_points) const;

  const xiiDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  xiiDeque<POINTTYPE>&       AccessPoints() { return m_Points; }

private:
  xiiDeque<POINTTYPE> m_Points;
  xiiDynamicOctree    m_Octree;
};

#include <GameEngine/AI/Implementation/PointOfInterestGraph_inl.h>
