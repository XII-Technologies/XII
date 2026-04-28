/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <SharedPluginScene/Common/Messages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedSceneProperty, 1, xiiRTTIDefaultAllocator<xiiExposedSceneProperty>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Object", m_Object)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedDocumentObjectPropertiesMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiExposedDocumentObjectPropertiesMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExportSceneGeometryMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiExportSceneGeometryMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Selection", m_bSelectionOnly),
    XII_MEMBER_PROPERTY("File", m_sOutputFile),
    XII_MEMBER_PROPERTY("Mode", m_iExtractionMode),
    XII_MEMBER_PROPERTY("Transform", m_Transform),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPullObjectStateMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiPullObjectStateMsgToEngine>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPushObjectStateData, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPushObjectStateData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LayerGuid", m_LayerGuid),
    XII_MEMBER_PROPERTY("Guid", m_ObjectGuid),
    XII_MEMBER_PROPERTY("Pos", m_vPosition),
    XII_MEMBER_PROPERTY("Rot", m_qRotation),
    XII_MAP_MEMBER_PROPERTY("Bones", m_BoneTransforms),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPushObjectStateMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiPushObjectStateMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("States", m_ObjectStates)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActiveLayerChangedMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiActiveLayerChangedMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ActiveLayer", m_ActiveLayer),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerVisibilityChangedMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiLayerVisibilityChangedMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("HiddenLayers", m_HiddenLayers),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
