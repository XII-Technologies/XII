#pragma once

#include <Foundation/Math/Vec3.h>
#include <GameEngine/GameEngineDLL.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template <typename POINTTYPE>
class xiiPointOfInterestGraph
{
public:
  void Initialize(const xiiVec3& center, const xiiVec3& halfExtents, float cellSize = 1.0f);

  POINTTYPE& AddPoint(const xiiVec3& position);

  void FindPointsOfInterest(const xiiVec3& position, float radius, xiiDynamicArray<xiiUInt32>& out_Points) const;

  const xiiDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  xiiDeque<POINTTYPE>&       AccessPoints() { return m_Points; }

private:
  xiiDeque<POINTTYPE> m_Points;
  xiiDynamicOctree    m_Octree;
};

#include <GameEngine/AI/Implementation/PointOfInterestGraph_inl.h>
