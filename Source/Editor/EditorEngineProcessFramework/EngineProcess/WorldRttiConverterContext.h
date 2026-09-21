/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/GuidHandleMap.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

/// The world rtti converter context tracks created objects and is capable of also handling components / game objects. Used by the xiiIPCObjectMirror to create / destroy objects.
///
/// As of now, it does not remove owner ptr when a parent is deleted, so it will accumulate zombie entries.
/// As requests to dead objects shouldn't generally happen this is for the time being not a problem.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiWorldRttiConverterContext : public xiiRttiConverterContext
{
public:
  virtual void Clear() override;
  void         DeleteExistingObjects();

  virtual xiiInternal::NewInstance<void> CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti) override;
  virtual void                           DeleteObject(const xiiUuid& guid) override;

  virtual void RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject) override;
  virtual void UnregisterObject(const xiiUuid& guid) override;

  virtual xiiRttiConverterObject GetObjectByGUID(const xiiUuid& guid) const override;
  virtual xiiUuid                GetObjectGUID(const xiiRTTI* pRtti, const void* pObject) const override;

  virtual void OnUnknownTypeError(xiiStringView sTypeName) override;

  xiiWorld*                                         m_pWorld = nullptr;
  xiiEditorGuidEngineHandleMap<xiiGameObjectHandle> m_GameObjectMap;
  xiiEditorGuidEngineHandleMap<xiiComponentHandle>  m_ComponentMap;

  xiiEditorGuidEngineHandleMap<xiiUInt32> m_OtherPickingMap;
  xiiEditorGuidEngineHandleMap<xiiUInt32> m_ComponentPickingMap;
  xiiUInt32                               m_uiNextComponentPickingID = 1;
  xiiUInt32                               m_uiHighlightID            = 1;

  struct Event
  {
    enum class Type
    {
      GameObjectCreated,
      GameObjectDeleted,
    };

    Type    m_Type;
    xiiUuid m_ObjectGuid;
  };

  xiiEvent<const Event&> m_Events;

  xiiSet<xiiString> m_UnknownTypes;
};
