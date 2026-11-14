/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GameEngine/Configuration/GameApplicationSettings.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QThread>
#include <ToolsFoundation/Document/DocumentManager.h>

namespace
{
  template <typename Predicate>
  bool PumpPlayModeUntil(Predicate predicate, int iMilliseconds)
  {
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < iMilliseconds)
    {
      QApplication::processEvents(QEventLoop::AllEvents, 20);
      QThread::msleep(1);
    }
    return predicate();
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorPlayMode);

XII_CREATE_SIMPLE_TEST(EditorPlayMode, PresentationAndStop)
{
  if (QGuiApplication::platformName() != "windows" && QGuiApplication::platformName() != "xcb")
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Play-mode capture requires native Qt windows/xcb.");
    return;
  }
  auto* editor = xiiQtEditorApp::GetSingleton();
  if (!XII_TEST_BOOL(editor != nullptr))
    return;
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/PlayModePreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor->ShutdownEditor());

  xiiStringBuilder project(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/PlayModeProject/xiiProject");
  if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, project)))
    return;
  xiiGameApplicationSettings presentation;
  presentation.m_bShowFrameStats = true;
  XII_TEST_RESULT(presentation.Save(":project/RuntimeConfigs/Presentation.ddl"));
  auto* connection = xiiEditorEngineProcessConnection::GetSingleton();
  if (!XII_TEST_RESULT(connection->RestartProcess()))
    return;

  xiiStringBuilder path(project.GetFileDirectory(), "/Play.xiiScene");
  auto*            scene = xiiDynamicCast<xiiSceneDocument*>(editor->CreateDocument(path, xiiDocumentFlags::RequestWindow));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  XII_SCOPE_EXIT(if (scene) scene->GetDocumentManager()->CloseDocument(scene));
  XII_TEST_STATUS(scene->SaveDocument());
  const auto loaded = scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10));
  XII_TEST_STATUS(loaded);
  if (loaded.Failed())
    return;
  auto* window = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(scene));
  if (!XII_TEST_BOOL(window && !window->GetViewWidgets().IsEmpty()))
    return;
  auto* view                             = window->GetViewWidgets()[0];
  view->m_pViewConfig->m_bShowFrameStats = false;
  view->SyncToEngine();
  PumpPlayModeUntil([] { return false; }, 500);

  const QDir screenshots(QString::fromUtf8(xiiTestFramework::GetInstance()->GetAbsOutputPath()) + "/Screenshots");
  for (xiiUInt32 iteration = 0; iteration < 2; ++iteration)
  {
    const auto previousImages = screenshots.entryList({"*.png"}, QDir::Files);
    scene->TriggerGameModePlay(false);
    if (!XII_TEST_BOOL(PumpPlayModeUntil([&] { return scene->GetGameMode() == GameMode::Play; }, 10000)))
      return;
    PumpPlayModeUntil([] { return false; }, 700);
    xiiConsoleCmdMsgToEngine capture;
    capture.m_iType    = 0;
    capture.m_sCommand = "TakeScreenshot()";
    XII_TEST_BOOL(connection->SendMessage(&capture));
    QString newImage;
    XII_TEST_BOOL(PumpPlayModeUntil([&] {
      for (const auto& image : screenshots.entryList({"*.png"}, QDir::Files))
        if (!previousImages.contains(image))
          newImage = screenshots.filePath(image);
      return !newImage.isEmpty();
    },
                                    10000));
    QImage image;
    if (XII_TEST_BOOL(PumpPlayModeUntil([&] { return image.load(newImage); }, 3000)))
    {
      xiiUInt32 greenPixels = 0;
      for (int y = qMax(0, image.height() - 40); y < image.height(); ++y)
        for (int x = 0; x < qMin(400, image.width()); ++x)
        {
          const auto pixel = image.pixel(x, y);
          if (qGreen(pixel) > qRed(pixel) + 25 && qGreen(pixel) > qBlue(pixel) + 25)
            ++greenPixels;
        }
      XII_TEST_BOOL_MSG(greenPixels > 20, "The game view must show the runtime FPS overlay while the editor viewport has it disabled.");
    }
    XII_TEST_BOOL(scene->StopGameMode());
    if (!XII_TEST_BOOL(PumpPlayModeUntil([&] { return scene->GetGameMode() == GameMode::Off; }, 10000)))
      return;
    XII_TEST_BOOL(!connection->IsProcessCrashed());
  }

  const auto identity = scene->GetGuid();
  scene->GetDocumentManager()->CloseDocument(scene);
  scene = xiiDynamicCast<xiiSceneDocument*>(editor->OpenDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  XII_TEST_BOOL(scene->GetGuid() == identity);
  XII_TEST_STATUS(scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10)));
  XII_TEST_BOOL(!connection->IsProcessCrashed());
}
