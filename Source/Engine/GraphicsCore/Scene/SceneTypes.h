/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Runtime classification used by scene queries and GPU visibility passes.
struct XII_GRAPHICSCORE_DLL xiiSceneObjectFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None          = 0,
    Enabled       = XII_BIT(0),
    Static        = XII_BIT(1),
    CastShadows   = XII_BIT(2),
    ReceiveShadows = XII_BIT(3),
    AlwaysVisible = XII_BIT(4),
    Occluder      = XII_BIT(5),
    Transparent   = XII_BIT(6),
    SensorVisible = XII_BIT(7),

    Default = Enabled | CastShadows | ReceiveShadows
  };

  struct Bits
  {
    StorageType Enabled : 1;
    StorageType Static : 1;
    StorageType CastShadows : 1;
    StorageType ReceiveShadows : 1;
    StorageType AlwaysVisible : 1;
    StorageType Occluder : 1;
    StorageType Transparent : 1;
    StorageType SensorVisible : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiSceneObjectFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneObjectFlags);

/// Generation checked identifier for an object in xiiSceneDatabase.
struct XII_GRAPHICSCORE_DLL xiiSceneObjectHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }
  XII_ALWAYS_INLINE void Invalidate()
  {
    m_uiIndex      = xiiInvalidIndex;
    m_uiGeneration = 0U;
  }

  XII_ALWAYS_INLINE bool operator==(const xiiSceneObjectHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiGeneration == rhs.m_uiGeneration; }
  XII_ALWAYS_INLINE bool operator!=(const xiiSceneObjectHandle& rhs) const { return !(*this == rhs); }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneObjectHandle);

/// Authoring description used to create a scene object. Geometry and material indices refer to
/// the global GPU geometry and material tables and therefore remain valid across views.
struct XII_GRAPHICSCORE_DLL xiiSceneObjectDesc
{
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsCenter() const { return m_LocalBounds.m_vCenter; }
  XII_ALWAYS_INLINE void SetLocalBoundsCenter(xiiVec3 value) { m_LocalBounds.m_vCenter = value; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsHalfExtents() const { return m_LocalBounds.m_vBoxHalfExtents; }
  XII_ALWAYS_INLINE void SetLocalBoundsHalfExtents(xiiVec3 value) { m_LocalBounds.m_vBoxHalfExtents = value; }
  [[nodiscard]] XII_ALWAYS_INLINE float GetLocalBoundsRadius() const { return m_LocalBounds.m_fSphereRadius; }
  XII_ALWAYS_INLINE void SetLocalBoundsRadius(float value) { m_LocalBounds.m_fSphereRadius = value; }

  xiiMat4                          m_LocalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere             m_LocalBounds    = xiiBoundingBoxSphere::MakeZero();
  xiiSceneObjectHandle             m_hParent;
  xiiBitflags<xiiSceneObjectFlags> m_Flags          = xiiSceneObjectFlags::Default;
  xiiUInt32                        m_uiGeometryIndex = xiiInvalidIndex;
  xiiUInt32                        m_uiMaterialIndex = xiiInvalidIndex;
  xiiUInt32                        m_uiVisibilityMask = 0xFFFFFFFFU;
  xiiUInt32                        m_uiUserData       = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneObjectDesc);

/// Canonical, backend independent GPU record. It deliberately contains both current and previous
/// transforms so motion vectors never need a second scene lookup.
struct XII_GRAPHICSCORE_DLL xiiGpuSceneInstance
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_GlobalTransform         = xiiMat4::MakeIdentity();
  xiiMat4   m_PreviousGlobalTransform = xiiMat4::MakeIdentity();
  xiiVec4   m_BoundsCenterRadius      = xiiVec4::MakeZero();
  xiiVec4   m_BoundsExtents           = xiiVec4::MakeZero();
  xiiUInt32 m_uiGeometryIndex         = xiiInvalidIndex;
  xiiUInt32 m_uiMaterialIndex         = xiiInvalidIndex;
  xiiUInt32 m_uiObjectIndex           = xiiInvalidIndex;
  xiiUInt32 m_uiFlags                 = 0U;
  xiiUInt32 m_uiVisibilityMask        = 0xFFFFFFFFU;
  xiiUInt32 m_uiUserData              = 0U;
  xiiUInt32 m_uiPadding0              = 0U;
  xiiUInt32 m_uiPadding1              = 0U;
};

static_assert(sizeof(xiiGpuSceneInstance) % 16U == 0U, "GPU scene records must preserve structured-buffer alignment.");

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuSceneInstance);

/// Contiguous dirty interval used for partial GPU uploads.
struct XII_GRAPHICSCORE_DLL xiiSceneUploadRange
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiFirstInstance = 0U;
  xiiUInt32 m_uiInstanceCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneUploadRange);

struct XII_GRAPHICSCORE_DLL xiiSceneDatabaseStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiObjectCount       = 0U;
  xiiUInt32 m_uiObjectCapacity    = 0U;
  xiiUInt32 m_uiDirtyObjectCount  = 0U;
  xiiUInt32 m_uiHierarchyDepth    = 0U;
  xiiUInt64 m_uiRevision          = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneDatabaseStats);
