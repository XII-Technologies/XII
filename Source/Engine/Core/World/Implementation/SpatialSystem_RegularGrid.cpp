#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Stopwatch.h>

xiiCVarInt cvar_SpatialQueriesCachingThreshold("Spatial.Queries.CachingThreshold", 100, xiiCVarFlags::Default, "Number of objects that are tested for a query before it is considered for caching");

namespace
{
  enum
  {
    MAX_CELL_INDEX  = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  XII_ALWAYS_INLINE xiiSimdVec4f ToVec3(const xiiSimdVec4i& v)
  {
    return v.ToFloat();
  }

  XII_ALWAYS_INLINE xiiSimdVec4i ToVec3I32(const xiiSimdVec4f& v)
  {
    xiiSimdVec4f vf = v.Floor();
    return xiiSimdVec4i::Truncate(vf);
  }

  XII_ALWAYS_INLINE xiiUInt64 GetCellKey(xiiInt32 x, xiiInt32 y, xiiInt32 z)
  {
    xiiUInt64 sx = (x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    xiiUInt64 sy = (y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    xiiUInt64 sz = (z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }

  XII_ALWAYS_INLINE xiiSimdBBox ComputeCellBoundingBox(const xiiSimdVec4i& vCellIndex, const xiiSimdVec4i& vCellSize)
  {
    xiiSimdVec4i overlapSize = vCellSize >> 2;
    xiiSimdVec4i minPos      = vCellIndex.CompMul(vCellSize);

    xiiSimdVec4f bmin = ToVec3(minPos - overlapSize);
    xiiSimdVec4f bmax = ToVec3(minPos + overlapSize + vCellSize);

    return xiiSimdBBox(bmin, bmax);
  }

  XII_ALWAYS_INLINE bool FilterByCategory(xiiUInt32 uiCategoryBitmask, xiiUInt32 uiQueryBitmask)
  {
    return (uiCategoryBitmask & uiQueryBitmask) == 0;
  }

  XII_ALWAYS_INLINE bool FilterByTags(const xiiTagSet& tags, const xiiTagSet& includeTags, const xiiTagSet& excludeTags)
  {
    if (!excludeTags.IsEmpty() && excludeTags.IsAnySet(tags))
      return true;

    if (!includeTags.IsEmpty() && !includeTags.IsAnySet(tags))
      return true;

    return false;
  }

  XII_ALWAYS_INLINE bool CanBeCached(xiiSpatialData::Category category)
  {
    return xiiSpatialData::GetCategoryFlags(category).IsSet(xiiSpatialData::Flags::FrequentChanges) == false;
  }

  void TagsToString(const xiiTagSet& tags, xiiStringBuilder& out_sSb)
  {
    out_sSb.Append("{ ");

    bool first = true;
    for (auto it = tags.GetIterator(); it.IsValid(); ++it)
    {
      if (!first)
      {
        out_sSb.Append(", ");
        first = false;
      }
      out_sSb.Append(it->GetTagString().GetView());
    }

    out_sSb.Append(" }");
  }

  struct PlaneData
  {
    xiiSimdVec4f m_x0x1x2x3;
    xiiSimdVec4f m_y0y1y2y3;
    xiiSimdVec4f m_z0z1z2z3;
    xiiSimdVec4f m_w0w1w2w3;

    xiiSimdVec4f m_x4x5x4x5;
    xiiSimdVec4f m_y4y5y4y5;
    xiiSimdVec4f m_z4z5z4z5;
    xiiSimdVec4f m_w4w5w4w5;
  };

  XII_FORCE_INLINE bool SphereFrustumIntersect(const xiiSimdBSphere& sphere, const PlaneData& planeData)
  {
    xiiSimdVec4f pos_xxxx(sphere.m_CenterAndRadius.x());
    xiiSimdVec4f pos_yyyy(sphere.m_CenterAndRadius.y());
    xiiSimdVec4f pos_zzzz(sphere.m_CenterAndRadius.z());
    xiiSimdVec4f pos_rrrr(sphere.m_CenterAndRadius.w());

    xiiSimdVec4f dot_0123;
    dot_0123 = xiiSimdVec4f::MulAdd(pos_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dot_0123 = xiiSimdVec4f::MulAdd(pos_yyyy, planeData.m_y0y1y2y3, dot_0123);
    dot_0123 = xiiSimdVec4f::MulAdd(pos_zzzz, planeData.m_z0z1z2z3, dot_0123);

    xiiSimdVec4f dot_4545;
    dot_4545 = xiiSimdVec4f::MulAdd(pos_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_4545 = xiiSimdVec4f::MulAdd(pos_yyyy, planeData.m_y4y5y4y5, dot_4545);
    dot_4545 = xiiSimdVec4f::MulAdd(pos_zzzz, planeData.m_z4z5z4z5, dot_4545);

    xiiSimdVec4b cmp_0123 = dot_0123 > pos_rrrr;
    xiiSimdVec4b cmp_4545 = dot_4545 > pos_rrrr;
    return (cmp_0123 || cmp_4545).NoneSet<4>();
  }

  XII_FORCE_INLINE xiiUInt32 SphereFrustumIntersect(const xiiSimdBSphere& sphereA, const xiiSimdBSphere& sphereB, const PlaneData& planeData)
  {
    xiiSimdVec4f posA_xxxx(sphereA.m_CenterAndRadius.x());
    xiiSimdVec4f posA_yyyy(sphereA.m_CenterAndRadius.y());
    xiiSimdVec4f posA_zzzz(sphereA.m_CenterAndRadius.z());
    xiiSimdVec4f posA_rrrr(sphereA.m_CenterAndRadius.w());

    xiiSimdVec4f dotA_0123;
    dotA_0123 = xiiSimdVec4f::MulAdd(posA_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotA_0123 = xiiSimdVec4f::MulAdd(posA_yyyy, planeData.m_y0y1y2y3, dotA_0123);
    dotA_0123 = xiiSimdVec4f::MulAdd(posA_zzzz, planeData.m_z0z1z2z3, dotA_0123);

    xiiSimdVec4f posB_xxxx(sphereB.m_CenterAndRadius.x());
    xiiSimdVec4f posB_yyyy(sphereB.m_CenterAndRadius.y());
    xiiSimdVec4f posB_zzzz(sphereB.m_CenterAndRadius.z());
    xiiSimdVec4f posB_rrrr(sphereB.m_CenterAndRadius.w());

    xiiSimdVec4f dotB_0123;
    dotB_0123 = xiiSimdVec4f::MulAdd(posB_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotB_0123 = xiiSimdVec4f::MulAdd(posB_yyyy, planeData.m_y0y1y2y3, dotB_0123);
    dotB_0123 = xiiSimdVec4f::MulAdd(posB_zzzz, planeData.m_z0z1z2z3, dotB_0123);

    xiiSimdVec4f posAB_xxxx = posA_xxxx.GetCombined<xiiSwizzle::XXXX>(posB_xxxx);
    xiiSimdVec4f posAB_yyyy = posA_yyyy.GetCombined<xiiSwizzle::XXXX>(posB_yyyy);
    xiiSimdVec4f posAB_zzzz = posA_zzzz.GetCombined<xiiSwizzle::XXXX>(posB_zzzz);
    xiiSimdVec4f posAB_rrrr = posA_rrrr.GetCombined<xiiSwizzle::XXXX>(posB_rrrr);

    xiiSimdVec4f dot_A45B45;
    dot_A45B45 = xiiSimdVec4f::MulAdd(posAB_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_A45B45 = xiiSimdVec4f::MulAdd(posAB_yyyy, planeData.m_y4y5y4y5, dot_A45B45);
    dot_A45B45 = xiiSimdVec4f::MulAdd(posAB_zzzz, planeData.m_z4z5z4z5, dot_A45B45);

    xiiSimdVec4b cmp_A0123  = dotA_0123 > posA_rrrr;
    xiiSimdVec4b cmp_B0123  = dotB_0123 > posB_rrrr;
    xiiSimdVec4b cmp_A45B45 = dot_A45B45 > posAB_rrrr;

    xiiSimdVec4b cmp_A45 = cmp_A45B45.Get<xiiSwizzle::XYXY>();
    xiiSimdVec4b cmp_B45 = cmp_A45B45.Get<xiiSwizzle::ZWZW>();

    xiiUInt32 result = (cmp_A0123 || cmp_A45).NoneSet<4>() ? 1 : 0;
    result |= (cmp_B0123 || cmp_B45).NoneSet<4>() ? 2 : 0;

    return result;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

struct CellDataMapping
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiCellIndex     = xiiInvalidIndex;
  xiiUInt32 m_uiCellDataIndex = xiiInvalidIndex;
};

struct xiiSpatialSystem_RegularGrid::Cell
{
  Cell(xiiAllocatorBase* pAlignedAlloctor, xiiAllocatorBase* pAllocator) :
    m_BoundingSpheres(pAlignedAlloctor), m_BoundingBoxHalfExtents(pAlignedAlloctor), m_TagSets(pAllocator), m_ObjectPointers(pAllocator), m_DataIndices(pAllocator)
  {
  }

  XII_FORCE_INLINE xiiUInt32 AddData(const xiiSimdBBoxSphere& bounds, const xiiTagSet& tags, xiiGameObject* pObject, xiiUInt64 uiLastVisibleFrameIdxAndVisType, xiiUInt32 uiDataIndex)
  {
    m_BoundingSpheres.PushBack(bounds.GetSphere());
    m_BoundingBoxHalfExtents.PushBack(bounds.m_BoxHalfExtents);
    m_TagSets.PushBack(tags);
    m_ObjectPointers.PushBack(pObject);
    m_DataIndices.PushBack(uiDataIndex);
    m_LastVisibleFrameIdxAndVisType.PushBack(uiLastVisibleFrameIdxAndVisType);

    return m_BoundingSpheres.GetCount() - 1;
  }

  // Returns the data index of the moved data
  XII_FORCE_INLINE xiiUInt32 RemoveData(xiiUInt32 uiCellDataIndex)
  {
    xiiUInt32 uiMovedDataIndex = m_DataIndices.PeekBack();

    m_BoundingSpheres.RemoveAtAndSwap(uiCellDataIndex);
    m_BoundingBoxHalfExtents.RemoveAtAndSwap(uiCellDataIndex);
    m_TagSets.RemoveAtAndSwap(uiCellDataIndex);
    m_ObjectPointers.RemoveAtAndSwap(uiCellDataIndex);
    m_DataIndices.RemoveAtAndSwap(uiCellDataIndex);
    m_LastVisibleFrameIdxAndVisType.RemoveAtAndSwap(uiCellDataIndex);

    XII_ASSERT_DEBUG(m_DataIndices.GetCount() == uiCellDataIndex || m_DataIndices[uiCellDataIndex] == uiMovedDataIndex, "Implementation error");

    return uiMovedDataIndex;
  }

  XII_ALWAYS_INLINE xiiBoundingBox GetBoundingBox() const { return xiiSimdConversion::ToBBoxSphere(m_Bounds).GetBox(); }

  xiiSimdBBoxSphere m_Bounds;

  xiiDynamicArray<xiiSimdBSphere>             m_BoundingSpheres;
  xiiDynamicArray<xiiSimdVec4f>               m_BoundingBoxHalfExtents;
  xiiDynamicArray<xiiTagSet>                  m_TagSets;
  xiiDynamicArray<xiiGameObject*>             m_ObjectPointers;
  mutable xiiDynamicArray<xiiAtomicInteger64> m_LastVisibleFrameIdxAndVisType;
  xiiDynamicArray<xiiUInt32>                  m_DataIndices;
};

//////////////////////////////////////////////////////////////////////////

struct CellKeyHashHelper
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiUInt64 value)
  {
    // return xiiUInt32(value * 2654435761U);
    return xiiHashHelper<xiiUInt64>::Hash(value);
  }

  XII_ALWAYS_INLINE static bool Equal(xiiUInt64 a, xiiUInt64 b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct xiiSpatialSystem_RegularGrid::Grid
{
  Grid(xiiSpatialSystem_RegularGrid& ref_system, xiiSpatialData::Category category) :
    m_System(ref_system), m_Cells(&ref_system.m_Allocator), m_CellKeyToCellIndex(&ref_system.m_Allocator), m_Category(category), m_bCanBeCached(CanBeCached(category))
  {
    xiiSimdBBox overflowBox;
    overflowBox.SetCenterAndHalfExtents(xiiSimdVec4f::ZeroVector(), xiiSimdVec4f((float)(ref_system.m_vCellSize.x() * MAX_CELL_INDEX)));

    auto pOverflowCell      = XII_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
    pOverflowCell->m_Bounds = overflowBox;

    m_Cells.PushBack(pOverflowCell);
  }

  xiiUInt32 GetOrCreateCell(const xiiSimdBBoxSphere& bounds)
  {
    xiiSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_System.m_fInvCellSize);
    xiiSimdBBox  cellBox   = ComputeCellBoundingBox(cellIndex, m_System.m_vCellSize);

    if (cellBox.Contains(bounds.GetBox()))
    {
      xiiUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

      xiiUInt32 uiCellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, uiCellIndex))
      {
        return uiCellIndex;
      }

      uiCellIndex = m_Cells.GetCount();
      m_CellKeyToCellIndex.Insert(cellKey, uiCellIndex);

      auto pNewCell      = XII_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
      pNewCell->m_Bounds = cellBox;

      m_Cells.PushBack(pNewCell);

      return uiCellIndex;
    }
    else
    {
      return m_uiOverflowCellIndex;
    }
  }

  void AddSpatialData(const xiiSimdBBoxSphere& bounds, const xiiTagSet& tags, xiiGameObject* pObject, xiiUInt64 uiLastVisibleFrameIdxAndVisType, const xiiSpatialDataHandle& hData)
  {
    xiiUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    xiiUInt32 uiCellIndex     = GetOrCreateCell(bounds);
    xiiUInt32 uiCellDataIndex = m_Cells[uiCellIndex]->AddData(bounds, tags, pObject, uiLastVisibleFrameIdxAndVisType, uiDataIndex);

    m_CellDataMappings.EnsureCount(uiDataIndex + 1);
    XII_ASSERT_DEBUG(m_CellDataMappings[uiDataIndex].m_uiCellIndex == xiiInvalidIndex, "data has already been added to a cell");
    m_CellDataMappings[uiDataIndex] = {uiCellIndex, uiCellDataIndex};
  }

  void RemoveSpatialData(const xiiSpatialDataHandle& hData)
  {
    xiiUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    auto&     mapping          = m_CellDataMappings[uiDataIndex];
    xiiUInt32 uiMovedDataIndex = m_Cells[mapping.m_uiCellIndex]->RemoveData(mapping.m_uiCellDataIndex);
    if (uiMovedDataIndex != uiDataIndex)
    {
      m_CellDataMappings[uiMovedDataIndex].m_uiCellDataIndex = mapping.m_uiCellDataIndex;
    }

    mapping = {};
  }

  bool MigrateSpatialDataFromOtherGrid(xiiUInt32 uiDataIndex, const Grid& other)
  {
    // Data has already been added
    if (uiDataIndex < m_CellDataMappings.GetCount() && m_CellDataMappings[uiDataIndex].m_uiCellIndex != xiiInvalidIndex)
      return false;

    auto& mapping = other.m_CellDataMappings[uiDataIndex];
    if (mapping.m_uiCellIndex == xiiInvalidIndex)
      return false;

    auto& pOtherCell = other.m_Cells[mapping.m_uiCellIndex];

    const xiiTagSet& tags = pOtherCell->m_TagSets[mapping.m_uiCellDataIndex];
    if (FilterByTags(tags, m_IncludeTags, m_ExcludeTags))
      return false;

    xiiSimdBBoxSphere bounds;
    bounds.m_CenterAndRadius                        = pOtherCell->m_BoundingSpheres[mapping.m_uiCellDataIndex].m_CenterAndRadius;
    bounds.m_BoxHalfExtents                         = pOtherCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex];
    xiiGameObject*  objectPointer                   = pOtherCell->m_ObjectPointers[mapping.m_uiCellDataIndex];
    const xiiUInt64 uiLastVisibleFrameIdxAndVisType = pOtherCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

    XII_ASSERT_DEBUG(pOtherCell->m_DataIndices[mapping.m_uiCellDataIndex] == uiDataIndex, "Implementation error");
    xiiSpatialDataHandle hData = xiiSpatialDataHandle(xiiSpatialDataId(uiDataIndex, 1));

    AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
    return true;
  }

  XII_ALWAYS_INLINE bool CachingCompleted() const { return m_uiLastMigrationIndex == xiiInvalidIndex; }

  template <typename Functor>
  XII_FORCE_INLINE void ForEachCellInBox(const xiiSimdBBox& box, Functor func) const
  {
    xiiSimdVec4i minIndex = ToVec3I32((box.m_Min - m_System.m_vOverlapSize) * m_System.m_fInvCellSize);
    xiiSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_System.m_vOverlapSize) * m_System.m_fInvCellSize);

    XII_ASSERT_DEBUG((minIndex.Abs() < xiiSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
    XII_ASSERT_DEBUG((maxIndex.Abs() < xiiSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

    const xiiInt32 iMinX = minIndex.x();
    const xiiInt32 iMinY = minIndex.y();
    const xiiInt32 iMinZ = minIndex.z();

    const xiiSimdVec4i diff           = maxIndex - minIndex + xiiSimdVec4i(1);
    const xiiInt32     iDiffX         = diff.x();
    const xiiInt32     iDiffY         = diff.y();
    const xiiInt32     iDiffZ         = diff.z();
    const xiiInt32     iNumIterations = iDiffX * iDiffY * iDiffZ;

    for (xiiInt32 i = 0; i < iNumIterations; ++i)
    {
      xiiInt32 index = i;
      xiiInt32 z     = i / (iDiffX * iDiffY);
      index -= z * iDiffX * iDiffY;
      xiiInt32 y = index / iDiffX;
      xiiInt32 x = index - (y * iDiffX);

      x += iMinX;
      y += iMinY;
      z += iMinZ;

      xiiUInt64 cellKey   = GetCellKey(x, y, z);
      xiiUInt32 cellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, cellIndex))
      {
        const Cell& constCell = *m_Cells[cellIndex];
        if (func(constCell) == xiiVisitorExecution::Stop)
          return;
      }
    }

    const Cell& overflowCell = *m_Cells[m_uiOverflowCellIndex];
    func(overflowCell);
  }

  xiiSpatialSystem_RegularGrid&       m_System;
  xiiDynamicArray<xiiUniquePtr<Cell>> m_Cells;

  xiiHashTable<xiiUInt64, xiiUInt32, CellKeyHashHelper> m_CellKeyToCellIndex;
  static constexpr xiiUInt32                            m_uiOverflowCellIndex = 0;

  xiiDynamicArray<CellDataMapping> m_CellDataMappings;

  const xiiSpatialData::Category m_Category;
  const bool                     m_bCanBeCached;

  xiiTagSet m_IncludeTags;
  xiiTagSet m_ExcludeTags;

  xiiUInt32 m_uiLastMigrationIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct xiiSpatialSystem_RegularGrid::Stats
{
  xiiUInt32 m_uiNumObjectsTested   = 0;
  xiiUInt32 m_uiNumObjectsPassed   = 0;
  xiiUInt32 m_uiNumObjectsFiltered = 0;
};

//////////////////////////////////////////////////////////////////////////

namespace xiiInternal
{
  struct QueryHelper
  {
    template <typename T>
    struct ShapeQueryData
    {
      T                               m_Shape;
      xiiSpatialSystem::QueryCallback m_Callback;
    };

    template <typename T, bool UseTagsFilter>
    static xiiVisitorExecution::Enum ShapeQueryCallback(const xiiSpatialSystem_RegularGrid::Cell& cell, const xiiSpatialSystem::QueryParams& queryParams, xiiSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, xiiVisibilityState visType)
    {
      auto pQueryData = static_cast<const ShapeQueryData<T>*>(pUserData);
      T    shape      = pQueryData->m_Shape;

      xiiSimdBBox cellBox = cell.m_Bounds.GetBox();
      if (!cellBox.Overlaps(shape))
        return xiiVisitorExecution::Continue;

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto tagSets         = cell.m_TagSets.GetData();
      auto objectPointers  = cell.m_ObjectPointers.GetData();

      const xiiUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      for (xiiUInt32 i = 0; i < numSpheres; ++i)
      {
        if (!shape.Overlaps(boundingSpheres[i]))
          continue;

        if constexpr (UseTagsFilter)
        {
          if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
          {
            ref_stats.m_uiNumObjectsFiltered++;
            continue;
          }
        }

        ref_stats.m_uiNumObjectsPassed++;

        if (pQueryData->m_Callback(objectPointers[i]) == xiiVisitorExecution::Stop)
          return xiiVisitorExecution::Stop;
      }

      return xiiVisitorExecution::Continue;
    }

    struct FrustumQueryData
    {
      PlaneData                              m_PlaneData;
      xiiDynamicArray<const xiiGameObject*>* m_pOutObjects;
      xiiUInt64                              m_uiFrameCounter;
      xiiSpatialSystem::IsOccludedFunc       m_IsOccludedCB;
    };

    template <bool UseTagsFilter, bool UseOcclusionCallback>
    static xiiVisitorExecution::Enum FrustumQueryCallback(const xiiSpatialSystem_RegularGrid::Cell& cell, const xiiSpatialSystem::QueryParams& queryParams, xiiSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, xiiVisibilityState visType)
    {
      auto      pQueryData = static_cast<FrustumQueryData*>(pUserData);
      PlaneData planeData  = pQueryData->m_PlaneData;

      xiiSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
      if (!SphereFrustumIntersect(cellSphere, planeData))
        return xiiVisitorExecution::Continue;

      if constexpr (UseOcclusionCallback)
      {
        if (pQueryData->m_IsOccludedCB(cell.m_Bounds.GetBox()))
        {
          return xiiVisitorExecution::Continue;
        }
      }

      xiiSimdBBox bbox;
      auto        boundingSpheres               = cell.m_BoundingSpheres.GetData();
      auto        boundingBoxHalfExtents        = cell.m_BoundingBoxHalfExtents.GetData();
      auto        tagSets                       = cell.m_TagSets.GetData();
      auto        objectPointers                = cell.m_ObjectPointers.GetData();
      auto        lastVisibleFrameIdxAndVisType = cell.m_LastVisibleFrameIdxAndVisType.GetData();

      const xiiUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      xiiUInt32       currentIndex      = 0;
      const xiiUInt64 uiFrameIdxAndType = (pQueryData->m_uiFrameCounter << 4) | static_cast<xiiUInt64>(visType);

      while (currentIndex < numSpheres)
      {
        if (numSpheres - currentIndex >= 32)
        {
          xiiUInt32 mask = 0;

          for (xiiUInt32 i = 0; i < 32; i += 2)
          {
            auto& objectSphereA = boundingSpheres[currentIndex + i + 0];
            auto& objectSphereB = boundingSpheres[currentIndex + i + 1];

            mask |= SphereFrustumIntersect(objectSphereA, objectSphereB, planeData) << i;
          }

          while (mask > 0)
          {
            xiiUInt32 i = xiiMath::FirstBitLow(mask) + currentIndex;
            mask &= mask - 1;

            if constexpr (UseTagsFilter)
            {
              if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
              {
                ref_stats.m_uiNumObjectsFiltered++;
                continue;
              }
            }

            if constexpr (UseOcclusionCallback)
            {
              bbox.SetCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);
              if (pQueryData->m_IsOccludedCB(bbox))
              {
                continue;
              }
            }

            lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
            pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

            ref_stats.m_uiNumObjectsPassed++;
          }

          currentIndex += 32;
        }
        else
        {
          xiiUInt32 i = currentIndex;
          ++currentIndex;

          if (!SphereFrustumIntersect(boundingSpheres[i], planeData))
            continue;

          if constexpr (UseTagsFilter)
          {
            if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
            {
              ref_stats.m_uiNumObjectsFiltered++;
              continue;
            }
          }

          if constexpr (UseOcclusionCallback)
          {
            bbox.SetCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);

            if (pQueryData->m_IsOccludedCB(bbox))
            {
              continue;
            }
          }

          lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
          pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

          ref_stats.m_uiNumObjectsPassed++;
        }
      }

      return xiiVisitorExecution::Continue;
    }
  };
} // namespace xiiInternal

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpatialSystem_RegularGrid, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSpatialSystem_RegularGrid::xiiSpatialSystem_RegularGrid(xiiUInt32 uiCellSize /*= 128*/) :
  m_AlignedAllocator("Spatial System Aligned", xiiFoundation::GetAlignedAllocator()), m_Grids(&m_Allocator), m_DataTable(&m_Allocator), m_vCellSize(uiCellSize), m_vOverlapSize(uiCellSize / 4.0f), m_fInvCellSize(1.0f / uiCellSize)
{
  XII_CHECK_AT_COMPILETIME(sizeof(Data) == 8);

  m_Grids.SetCount(MAX_NUM_GRIDS);

  cvar_SpatialQueriesCachingThreshold.m_CVarEvents.AddEventHandler([&](const xiiCVarEvent& e) {
    if (e.m_EventType == xiiCVarEvent::ValueChanged)
    {
      RemoveAllCachedGrids();
    } });
}

xiiSpatialSystem_RegularGrid::~xiiSpatialSystem_RegularGrid() = default;

xiiResult xiiSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const xiiSpatialDataHandle& hData, xiiBoundingBox& out_boundingBox) const
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return XII_FAILURE;

  ForEachGrid(*pData, hData,
              [&](Grid& ref_grid, const CellDataMapping& mapping) {
                auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

                out_boundingBox = pCell->GetBoundingBox();
                return xiiVisitorExecution::Stop;
              });

  return XII_SUCCESS;
}

template <>
struct xiiHashHelper<xiiBoundingBox>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiBoundingBox& value) { return xiiHashingUtils::xxHash32(&value, sizeof(xiiBoundingBox)); }

  XII_ALWAYS_INLINE static bool Equal(const xiiBoundingBox& a, const xiiBoundingBox& b) { return a == b; }
};

void xiiSpatialSystem_RegularGrid::GetAllCellBoxes(xiiDynamicArray<xiiBoundingBox>& out_boundingBoxes, xiiSpatialData::Category filterCategory /*= xiiInvalidSpatialDataCategory*/) const
{
  if (filterCategory != xiiInvalidSpatialDataCategory)
  {
    xiiUInt32 uiGridIndex = filterCategory.m_uiValue;
    auto&     pGrid       = m_Grids[uiGridIndex];
    if (pGrid != nullptr)
    {
      for (auto& pCell : pGrid->m_Cells)
      {
        out_boundingBoxes.ExpandAndGetRef() = pCell->GetBoundingBox();
      }
    }
  }
  else
  {
    xiiHashSet<xiiBoundingBox> boundingBoxes;

    for (auto& pGrid : m_Grids)
    {
      if (pGrid != nullptr)
      {
        for (auto& pCell : pGrid->m_Cells)
        {
          boundingBoxes.Insert(pCell->GetBoundingBox());
        }
      }
    }

    for (auto boundingBox : boundingBoxes)
    {
      out_boundingBoxes.PushBack(boundingBox);
    }
  }
}

void xiiSpatialSystem_RegularGrid::StartNewFrame()
{
  SUPER::StartNewFrame();

  m_SortedCacheCandidates.Clear();

  {
    XII_LOCK(m_CacheCandidatesMutex);

    for (xiiUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
    {
      auto& cacheCandidate = m_CacheCandidates[i];

      const float fScore = cacheCandidate.m_fQueryCount + cacheCandidate.m_fFilteredRatio * 100.0f;
      m_SortedCacheCandidates.PushBack({i, fScore});

      // Query has to be issued at least once every 10 frames to keep a stable value
      cacheCandidate.m_fQueryCount = xiiMath::Max(cacheCandidate.m_fQueryCount - 0.1f, 0.0f);
    }
  }

  m_SortedCacheCandidates.Sort();

  // First remove all cached grids that don't make it into the top MAX_NUM_CACHED_GRIDS to make space for new grids
  if (m_SortedCacheCandidates.GetCount() > MAX_NUM_CACHED_GRIDS)
  {
    for (xiiUInt32 i = MAX_NUM_CACHED_GRIDS; i < m_SortedCacheCandidates.GetCount(); ++i)
    {
      RemoveCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
    }
  }

  // Then take the MAX_NUM_CACHED_GRIDS candidates with the highest score and migrate the data
  for (xiiUInt32 i = 0; i < xiiMath::Min<xiiUInt32>(m_SortedCacheCandidates.GetCount(), MAX_NUM_CACHED_GRIDS); ++i)
  {
    MigrateCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
  }
}

xiiSpatialDataHandle xiiSpatialSystem_RegularGrid::CreateSpatialData(const xiiSimdBBoxSphere& bounds, xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return xiiSpatialDataHandle();

  return AddSpatialDataToGrids(bounds, pObject, uiCategoryBitmask, tags, false);
}

xiiSpatialDataHandle xiiSpatialSystem_RegularGrid::CreateSpatialDataAlwaysVisible(xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return xiiSpatialDataHandle();

  xiiSimdBBox hugeBox;
  hugeBox.SetCenterAndHalfExtents(xiiSimdVec4f::ZeroVector(), xiiSimdVec4f((float)(m_vCellSize.x() * MAX_CELL_INDEX)));

  return AddSpatialDataToGrids(hugeBox, pObject, uiCategoryBitmask, tags, true);
}

void xiiSpatialSystem_RegularGrid::DeleteSpatialData(const xiiSpatialDataHandle& hData)
{
  Data oldData;
  XII_VERIFY(m_DataTable.Remove(hData.GetInternalID(), &oldData), "Invalid spatial data handle");

  ForEachGrid(oldData, hData,
              [&](Grid& ref_grid, const CellDataMapping& mapping) {
                ref_grid.RemoveSpatialData(hData);
                return xiiVisitorExecution::Continue;
              });
}

void xiiSpatialSystem_RegularGrid::UpdateSpatialDataBounds(const xiiSpatialDataHandle& hData, const xiiSimdBBoxSphere& bounds)
{
  Data* pData = nullptr;
  XII_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  // No need to update bounds for always visible data
  if (IsAlwaysVisibleData(*pData))
    return;

  ForEachGrid(*pData, hData,
              [&](Grid& ref_grid, const CellDataMapping& mapping) {
                auto& pOldCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

                if (pOldCell->m_Bounds.GetBox().Contains(bounds.GetBox()))
                {
                  pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex]        = bounds.GetSphere();
                  pOldCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex] = bounds.m_BoxHalfExtents;
                }
                else
                {
                  const xiiTagSet tags          = pOldCell->m_TagSets[mapping.m_uiCellDataIndex];
                  xiiGameObject*  objectPointer = pOldCell->m_ObjectPointers[mapping.m_uiCellDataIndex];

                  const xiiUInt64 uiLastVisibleFrameIdxAndVisType = pOldCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

                  ref_grid.RemoveSpatialData(hData);

                  ref_grid.AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
                }

                return xiiVisitorExecution::Continue;
              });
}

void xiiSpatialSystem_RegularGrid::UpdateSpatialDataObject(const xiiSpatialDataHandle& hData, xiiGameObject* pObject)
{
  Data* pData = nullptr;
  XII_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  ForEachGrid(*pData, hData,
              [&](Grid& ref_grid, const CellDataMapping& mapping) {
                auto& pCell                                        = ref_grid.m_Cells[mapping.m_uiCellIndex];
                pCell->m_ObjectPointers[mapping.m_uiCellDataIndex] = pObject;
                return xiiVisitorExecution::Continue;
              });
}

void xiiSpatialSystem_RegularGrid::FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const
{
  XII_PROFILE_SCOPE("FindObjectsInSphere");

  xiiSimdBSphere simdSphere(xiiSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  xiiSimdBBox    simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<xiiSwizzle::WWWW>());

  xiiInternal::QueryHelper::ShapeQueryData<xiiSimdBSphere> queryData = {simdSphere, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
                                  &xiiInternal::QueryHelper::ShapeQueryCallback<xiiSimdBSphere, false>,
                                  &xiiInternal::QueryHelper::ShapeQueryCallback<xiiSimdBSphere, true>,
                                  &queryData, xiiVisibilityState::Indirect);
}

void xiiSpatialSystem_RegularGrid::FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const
{
  XII_PROFILE_SCOPE("FindObjectsInBox");

  xiiSimdBBox simdBox(xiiSimdConversion::ToVec3(box.m_vMin), xiiSimdConversion::ToVec3(box.m_vMax));

  xiiInternal::QueryHelper::ShapeQueryData<xiiSimdBBox> queryData = {simdBox, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
                                  &xiiInternal::QueryHelper::ShapeQueryCallback<xiiSimdBBox, false>,
                                  &xiiInternal::QueryHelper::ShapeQueryCallback<xiiSimdBBox, true>,
                                  &queryData, xiiVisibilityState::Indirect);
}

void xiiSpatialSystem_RegularGrid::FindVisibleObjects(const xiiFrustum& frustum, const QueryParams& queryParams, xiiDynamicArray<const xiiGameObject*>& out_Objects, xiiSpatialSystem::IsOccludedFunc IsOccluded, xiiVisibilityState visType) const
{
  XII_PROFILE_SCOPE("FindVisibleObjects");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiStopwatch timer;
#endif

  xiiVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints).AssertSuccess();

  xiiSimdVec4f simdCornerPoints[8];
  for (xiiUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = xiiSimdConversion::ToVec3(cornerPoints[i]);
  }

  xiiSimdBBox simdBox;
  simdBox.SetFromPoints(simdCornerPoints, 8);

  xiiInternal::QueryHelper::FrustumQueryData queryData;
  {
    // Compiler is too stupid to properly unroll a constant loop so we do it by hand
    xiiSimdVec4f plane0 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(0).m_vNormal.x)));
    xiiSimdVec4f plane1 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(1).m_vNormal.x)));
    xiiSimdVec4f plane2 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(2).m_vNormal.x)));
    xiiSimdVec4f plane3 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(3).m_vNormal.x)));
    xiiSimdVec4f plane4 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(4).m_vNormal.x)));
    xiiSimdVec4f plane5 = xiiSimdConversion::ToVec4(*reinterpret_cast<const xiiVec4*>(&(frustum.GetPlane(5).m_vNormal.x)));

    xiiSimdMat4f helperMat;
    helperMat.SetRows(plane0, plane1, plane2, plane3);

    queryData.m_PlaneData.m_x0x1x2x3 = helperMat.m_col0;
    queryData.m_PlaneData.m_y0y1y2y3 = helperMat.m_col1;
    queryData.m_PlaneData.m_z0z1z2z3 = helperMat.m_col2;
    queryData.m_PlaneData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    queryData.m_PlaneData.m_x4x5x4x5 = helperMat.m_col0;
    queryData.m_PlaneData.m_y4y5y4y5 = helperMat.m_col1;
    queryData.m_PlaneData.m_z4z5z4z5 = helperMat.m_col2;
    queryData.m_PlaneData.m_w4w5w4w5 = helperMat.m_col3;

    queryData.m_pOutObjects    = &out_Objects;
    queryData.m_uiFrameCounter = m_uiFrameCounter;

    queryData.m_IsOccludedCB = IsOccluded;
  }

  if (IsOccluded.IsValid())
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
                                    &xiiInternal::QueryHelper::FrustumQueryCallback<false, true>,
                                    &xiiInternal::QueryHelper::FrustumQueryCallback<true, true>,
                                    &queryData, visType);
  }
  else
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
                                    &xiiInternal::QueryHelper::FrustumQueryCallback<false, false>,
                                    &xiiInternal::QueryHelper::FrustumQueryCallback<true, false>,
                                    &queryData, visType);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_TimeTaken = timer.GetRunningTotal();
  }
#endif
}

xiiVisibilityState xiiSpatialSystem_RegularGrid::GetVisibilityState(const xiiSpatialDataHandle& hData, xiiUInt32 uiNumFramesBeforeInvisible) const
{
  Data* pData = nullptr;
  XII_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  if (IsAlwaysVisibleData(*pData))
    return xiiVisibilityState::Direct;

  xiiUInt64 uiLastVisibleFrameIdxAndVisType = 0;
  ForEachGrid(*pData, hData,
              [&](const Grid& grid, const CellDataMapping& mapping) {
                auto& pCell                     = grid.m_Cells[mapping.m_uiCellIndex];
                uiLastVisibleFrameIdxAndVisType = xiiMath::Max<xiiUInt64>(uiLastVisibleFrameIdxAndVisType, pCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex]);
                return xiiVisitorExecution::Continue;
              });

  const xiiUInt64 uiLastVisibleFrameIdx = (uiLastVisibleFrameIdxAndVisType >> 4);
  const xiiUInt64 uiLastVisibilityType  = (uiLastVisibleFrameIdxAndVisType & static_cast<xiiUInt64>(15)); // mask out lower 4 bits

  if (m_uiFrameCounter > uiLastVisibleFrameIdx + uiNumFramesBeforeInvisible)
    return xiiVisibilityState::Invisible;

  return static_cast<xiiVisibilityState>(uiLastVisibilityType);
}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
void xiiSpatialSystem_RegularGrid::GetInternalStats(xiiStringBuilder& sb) const
{
  sb = "Cache Candidates:\n";

  XII_LOCK(m_CacheCandidatesMutex);

  for (auto& sortedCandidate : m_SortedCacheCandidates)
  {
    auto& candidate = m_CacheCandidates[sortedCandidate.m_uiIndex];

    sb.AppendFormat(" \nCategory: {}\nInclude Tags: ", candidate.m_Category.m_uiValue);
    TagsToString(candidate.m_IncludeTags, sb);
    sb.Append("\nExclude Tags: ");
    TagsToString(candidate.m_ExcludeTags, sb);
    sb.AppendFormat("\nScore: {}", xiiArgF(sortedCandidate.m_fScore, 2));

    const xiiUInt32 uiGridIndex = candidate.m_uiGridIndex;
    if (uiGridIndex != xiiInvalidIndex)
    {
      auto& pGrid = m_Grids[uiGridIndex];
      if (pGrid->CachingCompleted())
      {
        sb.Append("\nReady to use!\n");
      }
      else
      {
        const xiiUInt32 uiNumObjectsMigrated = pGrid->m_uiLastMigrationIndex;
        sb.AppendFormat("\nMigration Status: {}%%\n", xiiArgF(float(uiNumObjectsMigrated) / m_DataTable.GetCount() * 100.0f, 2));
      }
    }
  }
}
#endif

XII_ALWAYS_INLINE bool xiiSpatialSystem_RegularGrid::IsAlwaysVisibleData(const Data& data) const
{
  return data.m_uiAlwaysVisible != 0;
}

xiiSpatialDataHandle xiiSpatialSystem_RegularGrid::AddSpatialDataToGrids(const xiiSimdBBoxSphere& bounds, xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags, bool bAlwaysVisible)
{
  Data data;
  data.m_uiGridBitmask   = uiCategoryBitmask;
  data.m_uiAlwaysVisible = bAlwaysVisible ? 1 : 0;

  // find matching cached grids and add them to data.m_uiGridBitmask
  for (xiiUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiCategoryBitmask) == 0 ||
        FilterByTags(tags, pGrid->m_IncludeTags, pGrid->m_ExcludeTags))
      continue;

    data.m_uiGridBitmask |= XII_BIT(uiCachedGridIndex);
  }

  auto hData = xiiSpatialDataHandle(m_DataTable.Insert(data));

  xiiUInt64 uiGridBitmask = data.m_uiGridBitmask;
  while (uiGridBitmask > 0)
  {
    xiiUInt32 uiGridIndex = xiiMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
    {
      pGrid = XII_NEW(&m_Allocator, Grid, *this, xiiSpatialData::Category(uiGridIndex));
    }

    pGrid->AddSpatialData(bounds, tags, pObject, m_uiFrameCounter, hData);
  }

  return hData;
}

template <typename Functor>
XII_FORCE_INLINE void xiiSpatialSystem_RegularGrid::ForEachGrid(const Data& data, const xiiSpatialDataHandle& hData, Functor func) const
{
  xiiUInt64 uiGridBitmask = data.m_uiGridBitmask;
  xiiUInt32 uiDataIndex   = hData.GetInternalID().m_InstanceIndex;

  while (uiGridBitmask > 0)
  {
    xiiUInt32 uiGridIndex = xiiMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& grid    = *m_Grids[uiGridIndex];
    auto& mapping = grid.m_CellDataMappings[uiDataIndex];

    if (func(grid, mapping) == xiiVisitorExecution::Stop)
      break;
  }
}

void xiiSpatialSystem_RegularGrid::ForEachCellInBoxInMatchingGrids(const xiiSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, xiiVisibilityState visType) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
  }
#endif

  xiiUInt32 uiGridBitmask = queryParams.m_uiCategoryBitmask;

  // search for cached grids that match the exact query params first
  for (xiiUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr || pGrid->CachingCompleted() == false)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiGridBitmask) == 0 ||
        pGrid->m_IncludeTags != queryParams.m_IncludeTags ||
        pGrid->m_ExcludeTags != queryParams.m_ExcludeTags)
      continue;

    uiGridBitmask &= ~pGrid->m_Category.GetBitmask();

    Stats stats;
    pGrid->ForEachCellInBox(box,
                            [&](const Cell& cell) {
                              return noFilterCallback(cell, queryParams, stats, pUserData, visType);
                            });

    UpdateCacheCandidate(queryParams.m_IncludeTags, queryParams.m_ExcludeTags, pGrid->m_Category, 0.0f);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }

  // then search for the rest
  const bool   useTagsFilter = queryParams.m_IncludeTags.IsEmpty() == false || queryParams.m_ExcludeTags.IsEmpty() == false;
  CellCallback cellCallback  = useTagsFilter ? filterByTagsCallback : noFilterCallback;

  while (uiGridBitmask > 0)
  {
    xiiUInt32 uiGridIndex = xiiMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
      continue;

    Stats stats;
    pGrid->ForEachCellInBox(box,
                            [&](const Cell& cell) {
                              return cellCallback(cell, queryParams, stats, pUserData, visType);
                            });

    if (pGrid->m_bCanBeCached && useTagsFilter)
    {
      const xiiUInt32 totalNumObjectsAfterSpatialTest = stats.m_uiNumObjectsFiltered + stats.m_uiNumObjectsPassed;
      const xiiUInt32 cacheThreshold                  = xiiUInt32(xiiMath::Max(cvar_SpatialQueriesCachingThreshold.GetValue(), 1));

      // 1.0 => all objects filtered, 0.0 => no object filtered by tags
      const float filteredRatio = float(double(stats.m_uiNumObjectsFiltered) / totalNumObjectsAfterSpatialTest);

      // Doesn't make sense to cache if there are only few objects in total or only few objects have been filtered
      if (totalNumObjectsAfterSpatialTest > cacheThreshold && filteredRatio > 0.1f)
      {
        UpdateCacheCandidate(queryParams.m_IncludeTags, queryParams.m_ExcludeTags, pGrid->m_Category, filteredRatio);
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }
}

void xiiSpatialSystem_RegularGrid::MigrateCachedGrid(xiiUInt32 uiCandidateIndex)
{
  xiiUInt32 uiTargetGridIndex = xiiInvalidIndex;
  xiiUInt32 uiSourceGridIndex = xiiInvalidIndex;

  {
    XII_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiTargetGridIndex    = cacheCandidate.m_uiGridIndex;
    uiSourceGridIndex    = cacheCandidate.m_Category.m_uiValue;

    if (uiTargetGridIndex == xiiInvalidIndex)
    {
      for (xiiUInt32 i = m_Grids.GetCount() - 1; i >= MAX_NUM_REGULAR_GRIDS; --i)
      {
        if (m_Grids[i] == nullptr)
        {
          uiTargetGridIndex = i;
          break;
        }
      }

      XII_ASSERT_DEBUG(uiTargetGridIndex != xiiInvalidIndex, "No free cached grid");
      cacheCandidate.m_uiGridIndex = uiTargetGridIndex;

      auto pGrid           = XII_NEW(&m_Allocator, Grid, *this, cacheCandidate.m_Category);
      pGrid->m_IncludeTags = cacheCandidate.m_IncludeTags;
      pGrid->m_ExcludeTags = cacheCandidate.m_ExcludeTags;

      m_Grids[uiTargetGridIndex] = pGrid;

      m_uiFirstCachedGridIndex = xiiMath::Min(m_uiFirstCachedGridIndex, uiTargetGridIndex);
    }
  }

  MigrateSpatialData(uiTargetGridIndex, uiSourceGridIndex);
}

void xiiSpatialSystem_RegularGrid::MigrateSpatialData(xiiUInt32 uiTargetGridIndex, xiiUInt32 uiSourceGridIndex)
{
  auto& pTargetGrid = m_Grids[uiTargetGridIndex];
  if (pTargetGrid->CachingCompleted())
    return;

  auto& pSourceGrid = m_Grids[uiSourceGridIndex];

  constexpr xiiUInt32 uiNumObjectsPerStep  = 64;
  xiiUInt32&          uiLastMigrationIndex = pTargetGrid->m_uiLastMigrationIndex;
  const xiiUInt32     uiSourceCount        = pSourceGrid->m_CellDataMappings.GetCount();
  const xiiUInt32     uiEndIndex           = xiiMath::Min(uiLastMigrationIndex + uiNumObjectsPerStep, uiSourceCount);

  for (xiiUInt32 i = uiLastMigrationIndex; i < uiEndIndex; ++i)
  {
    if (pTargetGrid->MigrateSpatialDataFromOtherGrid(i, *pSourceGrid))
    {
      m_DataTable.GetValueUnchecked(i).m_uiGridBitmask |= XII_BIT(uiTargetGridIndex);
    }
  }

  uiLastMigrationIndex = (uiEndIndex == uiSourceCount) ? xiiInvalidIndex : uiEndIndex;
}

void xiiSpatialSystem_RegularGrid::RemoveCachedGrid(xiiUInt32 uiCandidateIndex)
{
  xiiUInt32 uiGridIndex;

  {
    XII_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiGridIndex          = cacheCandidate.m_uiGridIndex;

    if (uiGridIndex == xiiInvalidIndex)
      return;

    cacheCandidate.m_fQueryCount    = 0.0f;
    cacheCandidate.m_fFilteredRatio = 0.0f;
    cacheCandidate.m_uiGridIndex    = xiiInvalidIndex;
  }

  m_Grids[uiGridIndex] = nullptr;
}

void xiiSpatialSystem_RegularGrid::RemoveAllCachedGrids()
{
  XII_LOCK(m_CacheCandidatesMutex);

  for (xiiUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
  {
    RemoveCachedGrid(i);
  }
}

void xiiSpatialSystem_RegularGrid::UpdateCacheCandidate(const xiiTagSet& includeTags, const xiiTagSet& excludeTags, xiiSpatialData::Category category, float filteredRatio) const
{
  XII_LOCK(m_CacheCandidatesMutex);

  CacheCandidate* pCacheCandiate = nullptr;
  for (auto& cacheCandidate : m_CacheCandidates)
  {
    if (cacheCandidate.m_Category == category &&
        cacheCandidate.m_IncludeTags == includeTags &&
        cacheCandidate.m_ExcludeTags == excludeTags)
    {
      pCacheCandiate = &cacheCandidate;
      break;
    }
  }

  if (pCacheCandiate != nullptr)
  {
    pCacheCandiate->m_fQueryCount    = xiiMath::Min(pCacheCandiate->m_fQueryCount + 1.0f, 100.0f);
    pCacheCandiate->m_fFilteredRatio = xiiMath::Max(pCacheCandiate->m_fFilteredRatio, filteredRatio);
  }
  else
  {
    m_CacheCandidates.PushBack({includeTags, excludeTags, category, 1, filteredRatio});
  }
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);
