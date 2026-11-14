/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

#include <Core/World/WorldModule.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Types/UniquePtr.h>

class xiiView;

/// Uses the existing document sync channel. Only GUIDs, visibility and configuration cross processes.
class XII_ENGINEPLUGINJOLT_DLL xiiJoltAuthoringSyncObject final : public xiiEditorEngineSyncObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltAuthoringSyncObject, xiiEditorEngineSyncObject);

public:
  xiiJoltAuthoringSyncObject();
  ~xiiJoltAuthoringSyncObject();
  void UpdateForEngine(xiiWorld* pWorld) override;

  xiiUuid   m_ComponentGuid; ///< Invalid for the document's visualization settings object.
  bool      m_bVisible        = true;
  bool      m_bAllShapes      = false;
  xiiUInt32 m_uiModes         = 0;
  xiiUInt32 m_uiLayerRevision = 0;

private:
  xiiWorld* m_pWorld = nullptr; ///< Engine context destroys synchronized objects before its world.
};

class XII_ENGINEPLUGINJOLT_DLL xiiJoltAuthoringWorldModule final : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltAuthoringWorldModule, xiiWorldModule);

public:
  explicit xiiJoltAuthoringWorldModule(xiiWorld* pWorld);
  ~xiiJoltAuthoringWorldModule();
  void Update(const xiiJoltAuthoringSyncObject& object);
  void Remove(xiiUuid guid);
  void Draw(const xiiView& view);

private:
  void WorldClear() override;
  struct State;
  xiiUniquePtr<State> m_pState;
};
