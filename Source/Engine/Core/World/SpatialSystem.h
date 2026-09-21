/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/Reflection.h>
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

  using QueryCallback = xiiDelegate<xiiVisitorExecution::Enum(xiiGameObject*)>;

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
    xiiUInt32        m_uiCategoryBitmask = 0;
    const xiiTagSet* m_pIncludeTags      = nullptr;
    const xiiTagSet* m_pExcludeTags      = nullptr;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;
#endif
  };

  virtual void FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_objects) const;
  virtual void FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_objects) const;
  virtual void FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = xiiDelegate<bool(const xiiSimdBBox&)>;

  virtual void FindVisibleObjects(const xiiFrustum& frustum, const QueryParams& queryParams, xiiDynamicArray<const xiiGameObject*>& out_objects, IsOccludedFunc isOccluded, xiiVisibilityState::Enum visType) const = 0;

  /// Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  virtual xiiVisibilityState::Enum GetVisibilityState(const xiiSpatialDataHandle& hData, xiiUInt32 uiNumFramesBeforeInvisible) const = 0;

  ///@}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(xiiStringBuilder& ref_sSb) const;
#endif

protected:
  xiiProxyAllocator m_Allocator;

  xiiUInt64 m_uiFrameCounter = 0;
};

class XII_CORE_DLL xiiScriptExtensionClass_Spatial
{
public:
  static xiiGameObject* FindClosestObjectInSphere(xiiWorld* pWorld, xiiStringView sCategory, const xiiVec3& vCenter, float fRadius);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Spatial);
