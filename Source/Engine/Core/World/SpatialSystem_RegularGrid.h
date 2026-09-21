/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/SpatialSystem.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/UniquePtr.h>

namespace xiiInternal
{
  struct QueryHelper;
}

class XII_CORE_DLL xiiSpatialSystem_RegularGrid : public xiiSpatialSystem
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpatialSystem_RegularGrid, xiiSpatialSystem);

public:
  xiiSpatialSystem_RegularGrid(xiiUInt32 uiCellSize = 128);
  ~xiiSpatialSystem_RegularGrid();

  /// Returns the bounding box of the cell associated with the given spatial data. Useful for debug visualizations.
  xiiResult GetCellBoxForSpatialData(const xiiSpatialDataHandle& hData, xiiBoundingBox& out_boundingBox) const;

  /// Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(xiiDynamicArray<xiiBoundingBox>& out_boundingBoxes, xiiSpatialData::Category filterCategory = xiiInvalidSpatialDataCategory) const;

private:
  friend xiiInternal::QueryHelper;

  // xiiSpatialSystem implementation
  virtual void StartNewFrame() override;

  xiiSpatialDataHandle CreateSpatialData(const xiiSimdBBoxSphere& bounds, xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags) override;
  xiiSpatialDataHandle CreateSpatialDataAlwaysVisible(xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags) override;

  void DeleteSpatialData(const xiiSpatialDataHandle& hData) override;

  void UpdateSpatialDataBounds(const xiiSpatialDataHandle& hData, const xiiSimdBBoxSphere& bounds) override;
  void UpdateSpatialDataObject(const xiiSpatialDataHandle& hData, xiiGameObject* pObject) override;

  void FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const override;
  void FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const override;

  void FindVisibleObjects(const xiiFrustum& frustum, const QueryParams& queryParams, xiiDynamicArray<const xiiGameObject*>& out_Objects, xiiSpatialSystem::IsOccludedFunc IsOccluded, xiiVisibilityState::Enum visType) const override;

  xiiVisibilityState::Enum GetVisibilityState(const xiiSpatialDataHandle& hData, xiiUInt32 uiNumFramesBeforeInvisible) const override;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(xiiStringBuilder& sb) const override;
#endif

  xiiProxyAllocator m_AlignedAllocator;

  xiiSimdVec4i m_vCellSize;
  xiiSimdVec4f m_vOverlapSize;
  xiiSimdFloat m_fInvCellSize;

  enum
  {
    MAX_NUM_GRIDS         = 63,
    MAX_NUM_REGULAR_GRIDS = (sizeof(xiiSpatialData::Category::m_uiValue) * 8),
    MAX_NUM_CACHED_GRIDS  = MAX_NUM_GRIDS - MAX_NUM_REGULAR_GRIDS
  };

  struct Cell;
  struct Grid;
  xiiDynamicArray<xiiUniquePtr<Grid>> m_Grids;
  xiiUInt32                           m_uiFirstCachedGridIndex = MAX_NUM_GRIDS;

  struct Data
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiGridBitmask : MAX_NUM_GRIDS;
    xiiUInt64 m_uiAlwaysVisible : 1;
  };

  xiiIdTable<xiiSpatialDataId, Data, xiiLocalAllocatorWrapper> m_DataTable;

  bool IsAlwaysVisibleData(const Data& data) const;

  xiiSpatialDataHandle AddSpatialDataToGrids(const xiiSimdBBoxSphere& bounds, xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags, bool bAlwaysVisible);

  template <typename Functor>
  void ForEachGrid(const Data& data, const xiiSpatialDataHandle& hData, Functor func) const;

  struct Stats;
  using CellCallback = xiiDelegate<xiiVisitorExecution::Enum(const Cell&, const QueryParams&, Stats&, void*, xiiVisibilityState::Enum)>;
  void ForEachCellInBoxInMatchingGrids(const xiiSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, xiiVisibilityState::Enum visType) const;

  struct CacheCandidate
  {
    xiiTagSet                m_IncludeTags;
    xiiTagSet                m_ExcludeTags;
    xiiSpatialData::Category m_Category;
    float                    m_fQueryCount    = 0.0f;
    float                    m_fFilteredRatio = 0.0f;
    xiiUInt32                m_uiGridIndex    = xiiInvalidIndex;
  };

  mutable xiiDynamicArray<CacheCandidate> m_CacheCandidates;
  mutable xiiMutex                        m_CacheCandidatesMutex;

  struct SortedCacheCandidate
  {
    xiiUInt32 m_uiIndex = 0;
    float     m_fScore  = 0;

    bool operator<(const SortedCacheCandidate& other) const
    {
      if (m_fScore != other.m_fScore)
        return m_fScore > other.m_fScore; // higher score comes first

      return m_uiIndex < other.m_uiIndex;
    }
  };

  xiiDynamicArray<SortedCacheCandidate> m_SortedCacheCandidates;

  void MigrateCachedGrid(xiiUInt32 uiCandidateIndex);
  void MigrateSpatialData(xiiUInt32 uiTargetGridIndex, xiiUInt32 uiSourceGridIndex);

  void RemoveCachedGrid(xiiUInt32 uiCandidateIndex);
  void RemoveAllCachedGrids();

  void UpdateCacheCandidate(const xiiTagSet* pIncludeTags, const xiiTagSet* pExcludeTags, xiiSpatialData::Category category, float filteredRatio) const;
};
