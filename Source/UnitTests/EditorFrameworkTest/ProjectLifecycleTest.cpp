/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <QApplication>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorProjectLifecycle);

XII_CREATE_SIMPLE_TEST(EditorProjectLifecycle, AssetsWithoutRestart)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Lifecycle");
  const xiiStringBuilder first(root, "/First/xiiProject"), second(root, "/Second/xiiProject");
  XII_TEST_STATUS(xiiToolsProject::CreateProject(first));
  const auto firstID = xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID;
  xiiToolsProject::CloseProject();
  XII_TEST_STATUS(xiiToolsProject::CreateProject(second));
  auto secondManifest               = xiiToolsProject::GetSingleton()->GetManifest();
  secondManifest.m_sAssetProfiles   = "Config/Profiles.ddl";
  secondManifest.m_sDataDirectories = "Config/Data.ddl";
  XII_TEST_STATUS(xiiToolsProject::GetSingleton()->SaveManifest(secondManifest));
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::SafeMode | xiiQtEditorApp::StartupFlags::NoRecent, root);
  const bool wasHeadless = xiiQtUiServices::IsHeadless();
  xiiQtUiServices::SetHeadless(false);
  XII_TEST_BOOL(!xiiQtUiServices::IsHeadless());
  xiiQtUiServices::SetHeadless(true);
  XII_TEST_BOOL(xiiQtUiServices::IsHeadless());
  XII_SCOPE_EXIT(xiiToolsProject::CloseProject(); QApplication::processEvents(); editor.ShutdownEditor(); xiiQtUiServices::SetHeadless(wasHeadless));
  auto* curator   = xiiAssetCurator::GetSingleton();
  auto* processor = xiiAssetProcessor::GetSingleton();
  XII_TEST_BOOL(!curator->IsReady());
  XII_TEST_BOOL(processor->QueueTransformAllAssets().Failed());
  xiiUInt32                                                     readyEvents = 0;
  xiiEvent<const xiiToolsProjectEvent&, xiiMutex>::Unsubscriber subscription;
  xiiToolsProject::s_Events.AddEventHandler([&](const xiiToolsProjectEvent& event) {
    if (event.m_Type == xiiToolsProjectEvent::Type::ProjectOpening)
    {
      XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
      XII_TEST_BOOL(processor->QueueTransformAllAssets().Failed());
    }
    if (event.m_Type == xiiToolsProjectEvent::Type::ProjectOpened)
    {
      ++readyEvents;
      XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
      XII_TEST_BOOL(curator->IsReady());
    }
  },
                                            subscription);

  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, first)))
    return;
  const xiiStringBuilder path(first.GetFileDirectory(), "Immediate.xiiSurfaceAsset");
  auto*                  document = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(document != nullptr))
    return;
  const auto assetID = document->GetGuid();
  XII_TEST_STATUS(document->SaveDocument());
  curator->NotifyOfFileChange(path);
  XII_TEST_BOOL(curator->TransformAsset(assetID, xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually).Succeeded());
  const xiiString output = document->GetAssetDocumentManager()->GetAbsoluteOutputFileName(document->GetAssetDocumentTypeDescriptor(), path, "");
  XII_TEST_BOOL(xiiOSFile::ExistsFile(output));
  xiiToolsProject::SaveProjectState();
  XII_TEST_BOOL(!xiiToolsProject::IsProjectClosing());
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(&assetID, 1), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  const auto firstGeneration = xiiToolsProject::GetProjectGeneration();
  // Both callbacks belong to First and must not close or open documents in Second.
  editor.CloseProject();
  editor.OpenDocumentQueued(path);
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, second)))
    return;
  QApplication::processEvents();
  XII_TEST_BOOL(xiiOSFile::ExistsFile(xiiStringBuilder(second.GetFileDirectory(), "Config/Profiles.ddl")));
  XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
  XII_TEST_BOOL(!xiiToolsProject::IsProjectGenerationCurrent(firstGeneration));
  XII_TEST_BOOL(xiiDocumentManager::GetDocumentByGuid(assetID) == nullptr);
  XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
  XII_TEST_INT(processor->GetTransformProgress().m_uiTotal, 0);
  xiiToolsProject::CloseProject();
  XII_TEST_BOOL(!curator->IsReady());
  XII_TEST_BOOL(processor->QueueTransformAllAssets().Failed());

  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, first)))
    return;
  XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == firstID);
  XII_TEST_BOOL(static_cast<bool>(curator->GetSubAsset(assetID)));
  XII_TEST_BOOL(curator->TransformAsset(assetID, xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually).Succeeded());
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(&assetID, 1), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(60);
  while (processor->GetTransformProgress().m_bActive && xiiTime::Now() < deadline)
  {
    curator->MainThreadTick(false);
    QApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  const auto progress = processor->GetTransformProgress();
  XII_TEST_BOOL(!progress.m_bActive);
  XII_TEST_INT(progress.m_uiCompleted, 1);
  XII_TEST_INT(progress.m_uiFailed, 0);
  processor->StopProcessTask(false, true);
  xiiFileReader compiled;
  XII_TEST_RESULT(compiled.Open(output));
  xiiAssetFileHeader header;
  XII_TEST_RESULT(header.Read(compiled));
  XII_TEST_BOOL(header.GetFileHash() != 0);
  compiled.Close();
  XII_TEST_INT(readyEvents, 3);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failed indexing setup rolls back and can be retried")
  {
    xiiToolsProject::CloseProject();
    xiiProjectManifest manifest;
    XII_TEST_STATUS(manifest.Load(second));
    manifest.m_sActiveAssetProfile = "MissingProfile";
    XII_TEST_STATUS(manifest.Save(second));
    {
      xiiTestLogInterface log;
      log.ExpectMessage("Project initialization failed", xiiLogMsgType::ErrorMsg);
      xiiLogSystemScope scope(&log);
      XII_TEST_BOOL(editor.CreateOrOpenProject(false, second).Failed());
    }
    XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
    XII_TEST_BOOL(!curator->IsReady());
    XII_TEST_BOOL(xiiToolsProject::GetSingleton() == nullptr);
    manifest.m_sActiveAssetProfile = "Default";
    XII_TEST_STATUS(manifest.Save(second));
    XII_TEST_RESULT(editor.CreateOrOpenProject(false, second));
    XII_TEST_BOOL(curator->IsReady());
    XII_TEST_INT(readyEvents, 4);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Required data-directory failure is recoverable")
  {
    xiiToolsProject::CloseProject();
    const xiiStringBuilder  config(second.GetFileDirectory(), secondManifest.m_sDataDirectories);
    constexpr xiiStringView invalid = "DataDir { string %Path { \">project/DoesNotExist\" } string %RootName { \"required\" } }";
    {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(config, xiiFileOpenMode::Write));
      XII_TEST_RESULT(file.Write(invalid.GetStartPointer(), invalid.GetElementCount()));
    }
    {
      xiiTestLogInterface log;
      log.ExpectMessage("Project initialization failed", xiiLogMsgType::ErrorMsg);
      xiiLogSystemScope scope(&log);
      XII_TEST_BOOL(editor.CreateOrOpenProject(false, second).Failed());
    }
    XII_TEST_BOOL(!curator->IsReady());
    XII_TEST_BOOL(xiiToolsProject::GetSingleton() == nullptr);
    XII_TEST_RESULT(xiiOSFile::DeleteFile(config));
    XII_TEST_RESULT(editor.CreateOrOpenProject(false, second));
    XII_TEST_BOOL(curator->IsReady());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document discovery uses the Editor readiness contract")
  {
    xiiToolsProject::CloseProject();
    auto* discovered = editor.OpenDocument(path, xiiDocumentFlags::None);
    if (XII_TEST_BOOL(discovered != nullptr))
      XII_TEST_BOOL(discovered->GetGuid() == assetID);
    XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
    XII_TEST_BOOL(curator->IsReady());
    XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == firstID);
  }
}
