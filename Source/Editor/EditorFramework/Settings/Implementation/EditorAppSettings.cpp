/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <EditorFramework/Settings/EditorSettings.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <EditorFramework/Platform/Windows/WindowsTaskbar.h>
#endif

void xiiQtEditorApp::SaveRecentFiles()
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");
}

void xiiQtEditorApp::LoadRecentFiles()
{
  m_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
  UpdateRecentProjects();
}

void xiiQtEditorApp::UpdateRecentProjects()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  if (m_pTaskbar && m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background | StartupFlags::NoRecent))
  {
    bool bHistoryChanged = false;
    m_pTaskbar->UpdateJumpList(m_RecentProjects, bHistoryChanged);
    // Windows clears its removal records after a successful commit. Persist the pruned
    // authoritative history so the next update cannot resurrect a removed destination.
    if (bHistoryChanged)
      m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  }
#endif
}

void xiiQtEditorApp::SaveOpenDocumentsList()
{
  xiiQtContainerWindow::GetContainerWindow()->SaveWindowLayout();
  const xiiDynamicArray<xiiQtDocumentWindow*>& windows = xiiQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  xiiRecentFilesList allDocs(windows.GetCount());

  xiiDynamicArray<xiiQtDocumentWindow*> allWindows;
  allWindows.Reserve(windows.GetCount());
  {
    auto*                                    container = xiiQtContainerWindow::GetContainerWindow();
    xiiHybridArray<xiiQtDocumentWindow*, 16> docWindows;
    container->GetDocumentWindows(docWindows);
    for (auto* pWindow : docWindows)
    {
      allWindows.PushBack(pWindow);
    }
  }
  for (xiiInt32 w = (xiiInt32)allWindows.GetCount() - 1; w >= 0; --w)
  {
    if (allWindows[w]->GetDocument())
    {
      allDocs.Insert(allWindows[w]->GetDocument()->GetDocumentPath(), 0);
    }
  }

  xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

xiiRecentFilesList xiiQtEditorApp::LoadOpenDocumentsList()
{
  xiiRecentFilesList allDocs(15);

  xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void xiiQtEditorApp::SaveSettings()
{
  // headless mode should never store any settings on disk
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  SaveRecentFiles();

  xiiEditorSettings::SaveApplicationSettings();

  // this setting is needed before we have loaded the preferences, so we duplicate it in the QSettings (registry)
  {
    xiiEditorPreferencesUser* pPreferences = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();

    QSettings s;
    s.beginGroup("EditorPreferences");
    s.setValue("ShowSplashScreen", pPreferences->m_bShowSplashScreen);
    s.endGroup();
  }

  if (xiiToolsProject::IsProjectOpen())
  {
    xiiEditorSettings::SaveProjectSettings();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save().IgnoreResult();
    GetRuntimePluginConfig(false).Save().IgnoreResult();
  }
}
