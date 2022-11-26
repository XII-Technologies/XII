#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void xiiQtEditorApp::SaveRecentFiles()
{
  m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");
}

void xiiQtEditorApp::LoadRecentFiles()
{
  m_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
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
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  SaveRecentFiles();

  xiiPreferences::SaveApplicationPreferences();

  // this setting is needed before we have loaded the preferences, so we duplicate it in the QSettings (registry)
  {
    xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

    QSettings s;
    s.beginGroup("EditorPreferences");
    s.setValue("ShowSplashscreen", pPreferences->m_bShowSplashscreen);
    s.endGroup();
  }

  if (xiiToolsProject::IsProjectOpen())
  {
    xiiPreferences::SaveProjectPreferences();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save().IgnoreResult();
    GetRuntimePluginConfig(false).Save().IgnoreResult();
  }
}
