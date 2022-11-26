#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/Types/TagSet.h>

class XII_CORE_DLL xiiSpatialSystem : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpatialSystem, xiiReflectedClass);

public:
  xiiSpatialSystem();
  ~xiiSpatialSystem();

  virtual void StartNewFrame();

  /// \name Spatial Data Functions
  ///@{

  virtual xiiSpatialDataHandle CreateSpatialData(const xiiSimdBBoxSphere& bounds, xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags) = 0;
  virtual xiiSpatialDataHandle CreateSpatialDataAlwaysVisible(xiiGameObject* pObject, xiiUInt32 uiCategoryBitmask, const xiiTagSet& tags)                     = 0;

  virtual void DeleteSpatialData(const xiiSpatialDataHandle& hData) = 0;

  virtual void UpdateSpatialDataBounds(const xiiSpatialDataHandle& hData, const xiiSimdBBoxSphere& bounds) = 0;
  virtual void UpdateSpatialDataObject(const xiiSpatialDataHandle& hData, xiiGameObject* pObject)          = 0;

  ///@}
  /// \name Simple Queries
  ///@{

  typedef xiiDelegate<xiiVisitorExecution::Enum(xiiGameObject*)> QueryCallback;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    xiiUInt32 m_uiTotalNumObjects  = 0; ///< The total number of spatial objects in this system.
    xiiUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    xiiUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    xiiTime   m_TimeTaken;              ///< Time taken to execute the query
  };
#endif

  struct QueryParams
  {
    xiiUInt32 m_uiCategoryBitmask = 0;
    xiiTagSet m_IncludeTags;
    xiiTagSet m_ExcludeTags;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;
#endif
  };

  virtual void FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_Objects) const;
  virtual void FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_Objects) const;
  virtual void FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = xiiDelegate<bool(const xiiSimdBBox&)>;

  virtual void FindVisibleObjects(const xiiFrustum& frustum, const QueryParams& queryParams, xiiDynamicArray<const xiiGameObject*>& out_Objects, IsOccludedFunc IsOccluded) const = 0;

  virtual xiiUInt64 GetNumFramesSinceVisible(const xiiSpatialDataHandle& hData) const = 0;

  ///@}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(xiiStringBuilder& sb) const;
#endif

protected:
  xiiProxyAllocator m_Allocator;

  xiiUInt64 m_uiFrameCounter = 0;
};
