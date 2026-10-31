/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_CREATE_SIMPLE_TEST_GROUP(ProjectLifecycle);

XII_CREATE_SIMPLE_TEST(ProjectLifecycle, Tools)
{
  const xiiStringBuilder first(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Projects/First/xiiProject");
  const xiiStringBuilder second(first.GetFileDirectory(), "../Second/xiiProject");
  xiiStringBuilder       cleanSecond(second);
  cleanSecond.MakeCleanPath();
  XII_TEST_RESULT(xiiOSFile::DeleteFile(first));
  XII_TEST_RESULT(xiiOSFile::DeleteFile(cleanSecond));
  xiiUInt32  opened = 0, failed = 0, closed = 0;
  bool       failInitialization = false;
  auto subscription       = xiiToolsProject::s_Events.AddEventHandler([&](const xiiToolsProjectEvent& event) {
    switch (event.m_Type)
    {
      case xiiToolsProjectEvent::Type::ProjectOpening:
        XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
        XII_TEST_BOOL(!xiiToolsProject::IsProjectClosing());
        XII_TEST_BOOL(event.m_pProject->GetState() == xiiToolsProject::State::Opening);
        XII_TEST_BOOL(event.m_pProject->GetManifest().m_ProjectID.IsValid());
        if (failInitialization)
          *event.m_pInitializationStatus = xiiStatus("Injected required subsystem failure.");
        break;
      case xiiToolsProjectEvent::Type::ProjectOpened:
        ++opened;
        XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
        XII_TEST_BOOL(!xiiToolsProject::IsProjectClosing());
        break;
      case xiiToolsProjectEvent::Type::ProjectOpenFailed:
        ++failed;
        XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
        XII_TEST_BOOL(event.m_pInitializationStatus->Failed());
        break;
      case xiiToolsProjectEvent::Type::ProjectClosing:
        XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
        XII_TEST_BOOL(xiiToolsProject::IsProjectClosing());
        break;
      case xiiToolsProjectEvent::Type::ProjectClosed:
        ++closed;
        XII_TEST_BOOL(event.m_pProject == nullptr);
        XII_TEST_BOOL(xiiToolsProject::GetSingleton() == nullptr);
        break;
      default:
        break;
    }
  });
  XII_SCOPE_EXIT(xiiToolsProject::CloseProject(); xiiToolsProject::s_Events.RemoveEventHandler(subscription));

  XII_TEST_STATUS(xiiToolsProject::CreateProject(first));
  const auto identity   = xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID;
  const auto generation = xiiToolsProject::GetProjectGeneration();
  XII_TEST_BOOL(xiiToolsProject::IsProjectGenerationCurrent(generation));
  xiiToolsProject::SaveProjectState();
  XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
  XII_TEST_BOOL(!xiiToolsProject::IsProjectClosing());
  XII_TEST_INT(xiiToolsProject::GetProjectGeneration(), generation);
  xiiToolsProject::CloseProject();
  XII_TEST_BOOL(!xiiToolsProject::IsProjectGenerationCurrent(generation));
  XII_TEST_STATUS(xiiToolsProject::OpenProject(first));
  XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == identity);
  XII_TEST_BOOL(xiiToolsProject::GetProjectGeneration() != generation);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Preflight failure preserves the active project")
  {
    const auto             currentGeneration = xiiToolsProject::GetProjectGeneration();
    const xiiStringBuilder invalid(first.GetFileDirectory(), "Invalid/xiiProject");
    {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(invalid, xiiFileOpenMode::Write));
      XII_TEST_RESULT(file.Write("NotProject {}", 13));
    }
    XII_TEST_BOOL(xiiToolsProject::OpenProject(invalid).Failed());
    XII_TEST_BOOL(xiiToolsProject::IsProjectGenerationCurrent(currentGeneration));
    XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == identity);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Switching and failed initialization clean up before retry")
  {
    XII_TEST_STATUS(xiiToolsProject::CreateProject(cleanSecond));
    XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID != identity);
    failInitialization = true;
    XII_TEST_BOOL(xiiToolsProject::OpenProject(first).Failed());
    XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
    XII_TEST_BOOL(xiiToolsProject::GetSingleton() == nullptr);
    XII_TEST_INT(failed, 1);
    failInitialization = false;
    XII_TEST_STATUS(xiiToolsProject::OpenProject(first));
    XII_TEST_BOOL(xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == identity);
    XII_TEST_INT(opened, 4);
    XII_TEST_INT(closed, 4);
    auto changed        = xiiToolsProject::GetSingleton()->GetManifest();
    changed.m_ProjectID = xiiUuid::MakeUuid();
    XII_TEST_BOOL(xiiToolsProject::GetSingleton()->SaveManifest(changed).Failed());
  }
}
