/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Scene/SceneTypes.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneObjectHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneObjectHandle>)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneObjectDesc, xiiNoBase, 4, xiiRTTIDefaultAllocator<xiiSceneObjectDesc>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("LocalTransform", m_LocalTransform),
      XII_ACCESSOR_PROPERTY("LocalBoundsCenter", GetLocalBoundsCenter, SetLocalBoundsCenter),
      XII_ACCESSOR_PROPERTY("LocalBoundsHalfExtents", GetLocalBoundsHalfExtents, SetLocalBoundsHalfExtents),
      XII_ACCESSOR_PROPERTY("LocalBoundsRadius", GetLocalBoundsRadius, SetLocalBoundsRadius),
      XII_MEMBER_PROPERTY("Parent", m_hParent),
      XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiGpuInstanceFlags, m_Flags),
      XII_MEMBER_PROPERTY("GeometryIndex", m_uiGeometryIndex),
      XII_MEMBER_PROPERTY("MaterialIndex", m_uiMaterialIndex),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("UserData", m_uiUserData),
      XII_MEMBER_PROPERTY("FirstPrimitive", m_uiFirstPrimitive),
      XII_MEMBER_PROPERTY("PrimitiveCount", m_uiPrimitiveCount),
      XII_MEMBER_PROPERTY("ForcedLod", m_uiForcedLod),
      XII_MEMBER_PROPERTY("DrawLod", m_uiDrawLod),
      XII_MEMBER_PROPERTY("LodBias", m_fLodBias),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneDatabaseStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneDatabaseStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ObjectCount", m_uiObjectCount),
      XII_MEMBER_PROPERTY("ObjectCapacity", m_uiObjectCapacity),
      XII_MEMBER_PROPERTY("DirtyObjectCount", m_uiDirtyObjectCount),
      XII_MEMBER_PROPERTY("HierarchyDepth", m_uiHierarchyDepth),
      XII_MEMBER_PROPERTY("Revision", m_uiRevision),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneTypes);
