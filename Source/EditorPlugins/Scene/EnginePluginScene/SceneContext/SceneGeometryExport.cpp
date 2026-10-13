/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <SharedPluginScene/Scene/SceneGeometryExporter.h>

void xiiSceneContext::HandleSceneGeometryMsg(const xiiExportSceneGeometryMsgToEngine* message)
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  xiiDynamicArray<xiiGameObjectHandle> selection;
  if (message->m_bSelectionOnly)
    for (const auto handle : m_SelectionWithChildren)
      selection.PushBack(handle);
  const auto                              status = xiiSceneGeometryExporter::ExportOBJ(*m_pWorld, message->m_sOutputFile,
                                                                                       static_cast<xiiSceneGeometryExportMode>(message->m_iExtractionMode), message->m_Transform,
                                                                                       message->m_bSelectionOnly, selection);
  xiiExportSceneGeometryResultMsgToEditor result;
  result.m_DocumentGuid = message->m_DocumentGuid;
  result.m_sOutputFile  = message->m_sOutputFile;
  result.m_bSucceeded   = status.Succeeded();
  result.m_sFailure     = status.GetMessageString();
  SendProcessMessage(&result);
}
