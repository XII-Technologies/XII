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
  }
  XII_END_PROPERTIES;
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
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneTypes);

