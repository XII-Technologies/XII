/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiWorld;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineSyncObject : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineSyncObject, xiiReflectedClass);

public:
  xiiEditorEngineSyncObject();
  ~xiiEditorEngineSyncObject();

  void Configure(xiiUuid ownerGuid, xiiDelegate<void(xiiEditorEngineSyncObject*)> onDestruction);

  xiiUuid GetDocumentGuid() const;
  void    SetModified(bool b = true) { m_bModified = b; }
  bool    GetModified() const { return m_bModified; }

  xiiUuid GetGuid() const { return m_SyncObjectGuid; }

  // One-time setup on the engine side.
  // \returns Whether the sync object is pickable via uiNextComponentPickingID.
  virtual bool SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID) { return false; }
  virtual void UpdateForEngine(xiiWorld* pWorld) {}

private:
  XII_ALLOW_PRIVATE_PROPERTIES(xiiEditorEngineSyncObject);

  friend class xiiAssetDocument;

  bool    m_bModified;
  xiiUuid m_SyncObjectGuid;
  xiiUuid m_OwnerGuid;

  xiiDelegate<void(xiiEditorEngineSyncObject*)> m_OnDestruction;
};
