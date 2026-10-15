/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>

/// Generation checked identifier for an object in xiiSceneDatabase.
using xiiSceneObjectId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiSceneObjectHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiSceneObjectHandle, xiiSceneObjectId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneObjectHandle);

/// Authoring description used to create a scene object. Geometry and material indices refer to
/// the global GPU geometry and material tables and therefore remain valid across views.
struct XII_GRAPHICSCORE_DLL xiiSceneObjectDesc
{
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsCenter() const { return m_LocalBounds.m_vCenter; }
  XII_ALWAYS_INLINE void                  SetLocalBoundsCenter(xiiVec3 value) { m_LocalBounds.m_vCenter = value; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsHalfExtents() const { return m_LocalBounds.m_vBoxHalfExtents; }
  XII_ALWAYS_INLINE void                  SetLocalBoundsHalfExtents(xiiVec3 value) { m_LocalBounds.m_vBoxHalfExtents = value; }
  [[nodiscard]] XII_ALWAYS_INLINE float   GetLocalBoundsRadius() const { return m_LocalBounds.m_fSphereRadius; }
  XII_ALWAYS_INLINE void                  SetLocalBoundsRadius(float value) { m_LocalBounds.m_fSphereRadius = value; }

  xiiMat4                          m_LocalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere             m_LocalBounds    = xiiBoundingBoxSphere::MakeZero();
  xiiSceneObjectHandle             m_hParent;
  xiiBitflags<xiiGpuInstanceFlags> m_Flags            = xiiGpuInstanceFlags::Default;
  xiiUInt32                        m_uiGeometryIndex  = xiiInvalidIndex;
  xiiUInt32                        m_uiMaterialIndex  = xiiInvalidIndex;
  xiiUInt32                        m_uiVisibilityMask = 0xFFFFFFFFU;
  xiiUInt32                        m_uiUserData       = 0U;
  xiiUInt32                        m_uiFirstPrimitive = 0U;
  xiiUInt32                        m_uiPrimitiveCount = 0U;              ///< Zero means the complete selected LOD.
  xiiUInt32                        m_uiForcedLod      = xiiInvalidIndex; ///< Automatic selection when invalid; otherwise requires this resident LOD.
  xiiUInt32                        m_uiDrawLod        = xiiInvalidIndex; ///< Candidate belongs to this LOD only; invalid admits any selected LOD.
  float                            m_fLodBias         = 0.0f;            ///< Octaves of projected coverage: +1 doubles coverage, favoring finer LODs.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneObjectDesc);

struct XII_GRAPHICSCORE_DLL xiiSceneDatabaseStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiObjectCount      = 0U;
  xiiUInt32 m_uiObjectCapacity   = 0U;
  xiiUInt32 m_uiDirtyObjectCount = 0U;
  xiiUInt32 m_uiHierarchyDepth   = 0U;
  xiiUInt64 m_uiRevision         = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneDatabaseStats);
