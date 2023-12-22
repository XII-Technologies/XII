#pragma once

#include <SharedPluginScene/SharedPluginSceneDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

class XII_SHAREDPLUGINSCENE_DLL xiiExposedSceneProperty : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedSceneProperty, xiiReflectedClass);

public:
  xiiString m_sName;
  xiiUuid   m_Object;
  xiiString m_sPropertyPath;
};

class XII_SHAREDPLUGINSCENE_DLL xiiExposedDocumentObjectPropertiesMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedDocumentObjectPropertiesMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiDynamicArray<xiiExposedSceneProperty> m_Properties;
};

class XII_SHAREDPLUGINSCENE_DLL xiiExportSceneGeometryMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExportSceneGeometryMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  bool      m_bSelectionOnly = false;
  xiiString m_sOutputFile;
  int       m_iExtractionMode; // xiiWorldGeoExtractionUtil::ExtractionMode
  xiiMat3   m_Transform;
};

class XII_SHAREDPLUGINSCENE_DLL xiiPullObjectStateMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPullObjectStateMsgToEngine, xiiEditorEngineDocumentMsg);
};

struct xiiPushObjectStateData
{
  xiiUuid                         m_LayerGuid;
  xiiUuid                         m_ObjectGuid;
  xiiVec3                         m_vPosition;
  xiiQuat                         m_qRotation;
  bool                            m_bAdjustFromPrefabRootChild = false; // only used internally, not synchronized
  xiiMap<xiiString, xiiTransform> m_BoneTransforms;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINSCENE_DLL, xiiPushObjectStateData);

class XII_SHAREDPLUGINSCENE_DLL xiiPushObjectStateMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPushObjectStateMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  xiiDynamicArray<xiiPushObjectStateData> m_ObjectStates;
};

class XII_SHAREDPLUGINSCENE_DLL xiiActiveLayerChangedMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActiveLayerChangedMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiUuid m_ActiveLayer;
};

class XII_SHAREDPLUGINSCENE_DLL xiiLayerVisibilityChangedMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerVisibilityChangedMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiHybridArray<xiiUuid, 1> m_HiddenLayers;
};
