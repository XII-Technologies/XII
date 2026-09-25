/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Scene/SceneTypes.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiSceneObjectFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiSceneObjectFlags::Enabled, xiiSceneObjectFlags::Static, xiiSceneObjectFlags::CastShadows, xiiSceneObjectFlags::ReceiveShadows)
  XII_BITFLAGS_CONSTANTS(xiiSceneObjectFlags::AlwaysVisible, xiiSceneObjectFlags::Occluder, xiiSceneObjectFlags::Transparent, xiiSceneObjectFlags::SensorVisible)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneObjectHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneObjectHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneObjectDesc, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneObjectDesc>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("LocalTransform", m_LocalTransform),
      XII_ACCESSOR_PROPERTY("LocalBoundsCenter", GetLocalBoundsCenter, SetLocalBoundsCenter),
      XII_ACCESSOR_PROPERTY("LocalBoundsHalfExtents", GetLocalBoundsHalfExtents, SetLocalBoundsHalfExtents),
      XII_ACCESSOR_PROPERTY("LocalBoundsRadius", GetLocalBoundsRadius, SetLocalBoundsRadius),
      XII_MEMBER_PROPERTY("Parent", m_hParent),
      XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiSceneObjectFlags, m_Flags),
      XII_MEMBER_PROPERTY("GeometryIndex", m_uiGeometryIndex),
      XII_MEMBER_PROPERTY("MaterialIndex", m_uiMaterialIndex),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("UserData", m_uiUserData),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSceneInstance, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiGpuSceneInstance>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("GlobalTransform", m_GlobalTransform),
      XII_MEMBER_PROPERTY("PreviousGlobalTransform", m_PreviousGlobalTransform),
      XII_MEMBER_PROPERTY("NormalTransformRow0", m_NormalTransformRow0),
      XII_MEMBER_PROPERTY("NormalTransformRow1", m_NormalTransformRow1),
      XII_MEMBER_PROPERTY("NormalTransformRow2", m_NormalTransformRow2),
      XII_MEMBER_PROPERTY("BoundsCenterRadius", m_BoundsCenterRadius),
      XII_MEMBER_PROPERTY("BoundsExtents", m_BoundsExtents),
      XII_MEMBER_PROPERTY("GeometryIndex", m_uiGeometryIndex),
      XII_MEMBER_PROPERTY("MaterialIndex", m_uiMaterialIndex),
      XII_MEMBER_PROPERTY("ObjectIndex", m_uiObjectIndex),
      XII_MEMBER_PROPERTY("Flags", m_uiFlags),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("UserData", m_uiUserData),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneUploadRange, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneUploadRange>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("FirstInstance", m_uiFirstInstance),
      XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
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
