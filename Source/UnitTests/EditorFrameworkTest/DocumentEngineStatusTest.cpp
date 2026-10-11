/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  class StatusTestDocument : public xiiAssetDocument
  {
  public:
    StatusTestDocument(xiiAssetDocEngineConnection connectionType) :
      xiiAssetDocument("DocumentEngineStatusTest.xiiAsset", XII_DEFAULT_NEW(xiiDocumentObjectManager), connectionType)
    {
      // Exercise IPC state without registering a project, opening a window, or launching an engine process.
      m_pDocumentInfo = CreateDocumentInfo();
      InitializeAfterLoading(false);
    }

    xiiTransformStatus InternalTransformAsset(xiiStreamWriter&, xiiStringView, const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags>) override
    {
      return xiiTransformStatus("This test document has no transform implementation.");
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorDocuments);

XII_CREATE_SIMPLE_TEST(EditorDocuments, EngineStatus)
{
  xiiEditorEngineProcessConnection connection;
  StatusTestDocument               unsupported(xiiAssetDocEngineConnection::None);
  XII_TEST_BOOL(unsupported.GetEngineStatus() == xiiAssetDocument::EngineStatus::Unsupported);
  XII_TEST_BOOL(unsupported.WaitForEngineStatusLoaded().Failed());
  xiiDocumentClearMsgToEngine message;
  XII_TEST_BOOL(!unsupported.SendMessageToEngine(&message));

  StatusTestDocument document(xiiAssetDocEngineConnection::Simple);
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Disconnected);
  XII_TEST_BOOL(document.WaitForEngineStatusLoaded().Failed());
  xiiEditorEngineSyncObject sync;
  document.AddSyncObject(&sync);
  sync.SetModified(false);

  xiiUInt32 acknowledgements = 0;
  auto      subscription     = document.m_ProcessMessageEvent.AddEventHandler([&](const xiiEditorEngineDocumentMsg* msg) {
    if (msg->IsInstanceOf<xiiDocumentOpenResponseMsgToEditor>())
      ++acknowledgements;
  });
  XII_SCOPE_EXIT(document.m_ProcessMessageEvent.RemoveEventHandler(subscription));

  xiiEditorEngineProcessConnection::Event processEvent;
  processEvent.m_Type = xiiEditorEngineProcessConnection::Event::Type::ProcessStarted;
  connection.s_Events.Broadcast(processEvent);
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Initializing);

  xiiDocumentOpenResponseMsgToEditor response;
  response.m_DocumentGuid = document.GetGuid();
  response.m_sFailure     = "No engine context is registered for this document type.";
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("Engine rejected document", xiiLogMsgType::ErrorMsg);
    document.HandleEngineMessage(&response);
  }
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Failed);
  auto failure = document.WaitForEngineStatusLoaded();
  XII_TEST_BOOL(failure.Failed());
  XII_TEST_BOOL(failure.GetMessageString().FindSubString(response.m_sFailure) != nullptr);
  XII_TEST_BOOL(!sync.GetModified());
  XII_TEST_INT(acknowledgements, 1);

  // The same document can recover on a later engine restart, without replacing its sync objects.
  connection.s_Events.Broadcast(processEvent);
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Initializing);
  response.m_bSucceeded = true;
  response.m_sFailure.Clear();
  document.HandleEngineMessage(&response);
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Loaded);
  XII_TEST_STATUS(document.WaitForEngineStatusLoaded());
  XII_TEST_BOOL(sync.GetModified());
  XII_TEST_INT(acknowledgements, 2);

  processEvent.m_Type = xiiEditorEngineProcessConnection::Event::Type::ProcessShutdown;
  connection.s_Events.Broadcast(processEvent);
  XII_TEST_BOOL(document.GetEngineStatus() == xiiAssetDocument::EngineStatus::Disconnected);
  XII_TEST_BOOL(document.WaitForEngineStatusLoaded().Failed());

  // A rejected full-mirroring document must not touch its unconnected mirror or send document objects.
  StatusTestDocument mirrored(xiiAssetDocEngineConnection::FullObjectMirroring);
  response.m_bSucceeded   = false;
  response.m_DocumentGuid = mirrored.GetGuid();
  response.m_sFailure     = "No context registered.";
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("Engine rejected document", xiiLogMsgType::ErrorMsg);
    mirrored.HandleEngineMessage(&response);
  }
  XII_TEST_BOOL(mirrored.GetEngineStatus() == xiiAssetDocument::EngineStatus::Failed);
}
