/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Baking/BakeSceneProxyOp.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpProxy_BakeScene, 1, xiiRTTIDefaultAllocator<xiiLongOpProxy_BakeScene>);
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLongOpProxy_BakeScene::InitializeRegistered(const xiiUuid& documentGuid, const xiiUuid& componentGuid)
{
  m_DocumentGuid  = documentGuid;
  m_ComponentGuid = componentGuid;
}

void xiiLongOpProxy_BakeScene::GetReplicationInfo(xiiStringBuilder& out_sReplicationOpType, xiiStreamWriter& ref_description)
{
  out_sReplicationOpType = "xiiLongOpWorker_BakeScene";

  xiiStringBuilder sOutputPath;
  sOutputPath.SetFormat(":project/AssetCache/Generated/{0}", m_ComponentGuid);
  ref_description << sOutputPath;
}

void xiiLongOpProxy_BakeScene::Finalize(xiiResult result, const xiiDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    xiiQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
