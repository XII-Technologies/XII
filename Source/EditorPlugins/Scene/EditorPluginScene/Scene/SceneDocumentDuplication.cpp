/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/ConversionUtils.h>

void xiiSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  xiiVec3 selectionSize = xiiVec3::MakeZero();
  if (GetEngineStatus() == EngineStatus::Loaded)
  {
    SendObjectSelection();
    xiiQuerySelectionBBoxMsgToEngine query;
    query.m_uiViewID   = xiiInvalidIndex;
    query.m_iPurpose   = 1;
    auto receiveBounds = [&](xiiProcessMessage* message) {
      const auto& result = *static_cast<const xiiQuerySelectionBBoxResultMsgToEditor*>(message);
      if (result.m_iPurpose != query.m_iPurpose)
        return false;
      if (result.m_vHalfExtents.IsValid())
        selectionSize = result.m_vHalfExtents * 2.0f;
      return true;
    };
    xiiProcessCommunicationChannel::WaitForMessageCallback callback = receiveBounds;
    if (SendMessageToEngine(&query))
      xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), xiiGetStaticRTTI<xiiQuerySelectionBBoxResultMsgToEditor>(), xiiTime::MakeFromSeconds(1), &callback).IgnoreResult();
  }
  xiiQtDuplicateDlg dlg(nullptr, selectionSize);
  if (dlg.exec() == QDialog::Rejected)
    return;

  xiiMap<xiiUuid, xiiUuid> parents;

  xiiAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  xiiStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", xiiConversionUtils::ToString(it.Key(), tmp1), xiiConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  xiiAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  xiiDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat         = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes             = temp;
  cmd.m_uiNumberOfCopies         = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation    = dlg.s_vRotationStep;
  cmd.m_vRandomRotation          = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation       = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates         = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis             = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius           = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle        = xiiAngle::MakeFromDegree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep         = xiiAngle::MakeFromDegree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}
