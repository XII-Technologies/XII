#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/SourceGen/CppProject.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

xiiActionDescriptorHandle xiiProjectActions::s_hEditorMenu;

xiiActionDescriptorHandle xiiProjectActions::s_hDocumentCategory;
xiiActionDescriptorHandle xiiProjectActions::s_hCreateDocument;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenDocument;
xiiActionDescriptorHandle xiiProjectActions::s_hRecentDocuments;

xiiActionDescriptorHandle xiiProjectActions::s_hProjectCategory;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenDashboard;
xiiActionDescriptorHandle xiiProjectActions::s_hCreateProject;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenProject;
xiiActionDescriptorHandle xiiProjectActions::s_hRecentProjects;
xiiActionDescriptorHandle xiiProjectActions::s_hCloseProject;
xiiActionDescriptorHandle xiiProjectActions::s_hDocsAndCommunity;

xiiActionDescriptorHandle xiiProjectActions::s_hSettingsCategory;
xiiActionDescriptorHandle xiiProjectActions::s_hEditorSettingsMenu;
xiiActionDescriptorHandle xiiProjectActions::s_hProjectSettingsMenu;
xiiActionDescriptorHandle xiiProjectActions::s_hShortcutEditor;
xiiActionDescriptorHandle xiiProjectActions::s_hDataDirectories;
xiiActionDescriptorHandle xiiProjectActions::s_hWindowConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hInputConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hPreferencesDlg;
xiiActionDescriptorHandle xiiProjectActions::s_hTagsDlg;
xiiActionDescriptorHandle xiiProjectActions::s_hImportAsset;
xiiActionDescriptorHandle xiiProjectActions::s_hAssetProfiles;
xiiActionDescriptorHandle xiiProjectActions::s_hExportProject;
xiiActionDescriptorHandle xiiProjectActions::s_hPluginSelection;
xiiActionDescriptorHandle xiiProjectActions::s_hClearAssetCaches;

xiiActionDescriptorHandle xiiProjectActions::s_hToolsMenu;
xiiActionDescriptorHandle xiiProjectActions::s_hToolsCategory;
xiiActionDescriptorHandle xiiProjectActions::s_hReloadResources;
xiiActionDescriptorHandle xiiProjectActions::s_hReloadEngine;
xiiActionDescriptorHandle xiiProjectActions::s_hLaunchFileserve;
xiiActionDescriptorHandle xiiProjectActions::s_hLaunchInspector;
xiiActionDescriptorHandle xiiProjectActions::s_hSaveProfiling;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenVsCode;

xiiActionDescriptorHandle xiiProjectActions::s_hCppProjectMenu;
xiiActionDescriptorHandle xiiProjectActions::s_hSetupCppProject;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenCppProject;
xiiActionDescriptorHandle xiiProjectActions::s_hCompileCppProject;

void xiiProjectActions::RegisterActions()
{
  s_hEditorMenu = XII_REGISTER_MENU("Menu.Editor");

  s_hDocumentCategory = XII_REGISTER_CATEGORY("DocumentCategory");
  s_hCreateDocument   = XII_REGISTER_ACTION_1("Document.Create", xiiActionScope::Global, "Project", "Ctrl+N", xiiProjectAction, xiiProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument     = XII_REGISTER_ACTION_1("Document.Open", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments  = XII_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", xiiRecentDocumentsMenuAction, "");

  s_hProjectCategory = XII_REGISTER_CATEGORY("ProjectCategory");
  s_hOpenDashboard   = XII_REGISTER_ACTION_1("Editor.OpenDashboard", xiiActionScope::Global, "Editor", "Ctrl+Shift+D", xiiProjectAction, xiiProjectAction::ButtonType::OpenDashboard);
  s_hCreateProject   = XII_REGISTER_ACTION_1("Project.Create", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CreateProject);
  s_hOpenProject     = XII_REGISTER_ACTION_1("Project.Open", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::OpenProject);
  s_hRecentProjects  = XII_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", xiiRecentProjectsMenuAction, "");
  s_hCloseProject    = XII_REGISTER_ACTION_1("Project.Close", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CloseProject);

  s_hSettingsCategory    = XII_REGISTER_CATEGORY("SettingsCategory");
  s_hEditorSettingsMenu  = XII_REGISTER_MENU_WITH_ICON("Menu.EditorSettings", ":/GuiFoundation/Icons/Settings16.png");
  s_hProjectSettingsMenu = XII_REGISTER_MENU("Menu.ProjectSettings");

  s_hShortcutEditor  = XII_REGISTER_ACTION_1("Editor.Shortcuts", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg  = XII_REGISTER_ACTION_1("Editor.Preferences", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::PreferencesDialog);
  s_hTagsDlg         = XII_REGISTER_ACTION_1("Engine.Tags", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::TagsDialog);
  s_hPluginSelection = XII_REGISTER_ACTION_1("Project.PluginSelection", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::PluginSelection);

  s_hDataDirectories  = XII_REGISTER_ACTION_1("Project.DataDirectories", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::DataDirectories);
  s_hInputConfig      = XII_REGISTER_ACTION_1("Project.InputConfig", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::InputConfig);
  s_hWindowConfig     = XII_REGISTER_ACTION_1("Project.WindowConfig", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::WindowConfig);
  s_hImportAsset      = XII_REGISTER_ACTION_1("Project.ImportAsset", xiiActionScope::Global, "Project", "Ctrl+I", xiiProjectAction, xiiProjectAction::ButtonType::ImportAsset);
  s_hAssetProfiles    = XII_REGISTER_ACTION_1("Project.AssetProfiles", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::AssetProfiles);
  s_hExportProject    = XII_REGISTER_ACTION_1("Project.ExportProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::ExportProject);
  s_hClearAssetCaches = XII_REGISTER_ACTION_1("Project.ClearAssetCaches", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::ClearAssetCaches);

  s_hToolsMenu       = XII_REGISTER_MENU("Menu.Tools");
  s_hToolsCategory   = XII_REGISTER_CATEGORY("ToolsCategory");
  s_hReloadResources = XII_REGISTER_ACTION_1("Engine.ReloadResources", xiiActionScope::Global, "Engine", "F4", xiiProjectAction, xiiProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine    = XII_REGISTER_ACTION_1("Engine.ReloadEngine", xiiActionScope::Global, "Engine", "Ctrl+Shift+F4", xiiProjectAction, xiiProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = XII_REGISTER_ACTION_1("Editor.LaunchFileserve", xiiActionScope::Global, "Engine", "", xiiProjectAction, xiiProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = XII_REGISTER_ACTION_1("Editor.LaunchInspector", xiiActionScope::Global, "Engine", "", xiiProjectAction, xiiProjectAction::ButtonType::LaunchInspector);
  s_hSaveProfiling   = XII_REGISTER_ACTION_1("Editor.SaveProfiling", xiiActionScope::Global, "Engine", "Ctrl+Alt+P", xiiProjectAction, xiiProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode      = XII_REGISTER_ACTION_1("Editor.OpenVsCode", xiiActionScope::Global, "Project", "Ctrl+Alt+O", xiiProjectAction, xiiProjectAction::ButtonType::OpenVsCode);

  s_hCppProjectMenu    = XII_REGISTER_MENU("Project.Cpp");
  s_hSetupCppProject   = XII_REGISTER_ACTION_1("Project.SetupCppProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::SetupCppProject);
  s_hOpenCppProject    = XII_REGISTER_ACTION_1("Project.OpenCppProject", xiiActionScope::Global, "Project", "Ctrl+Shift+O", xiiProjectAction, xiiProjectAction::ButtonType::OpenCppProject);
  s_hCompileCppProject = XII_REGISTER_ACTION_1("Project.CompileCppProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CompileCppProject);

  s_hDocsAndCommunity = XII_REGISTER_ACTION_1("Editor.DocsAndCommunity", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::ShowDocsAndCommunity);
}

void xiiProjectActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hEditorMenu);
  xiiActionManager::UnregisterAction(s_hDocumentCategory);
  xiiActionManager::UnregisterAction(s_hCreateDocument);
  xiiActionManager::UnregisterAction(s_hOpenDocument);
  xiiActionManager::UnregisterAction(s_hRecentDocuments);
  xiiActionManager::UnregisterAction(s_hProjectCategory);
  xiiActionManager::UnregisterAction(s_hOpenDashboard);
  xiiActionManager::UnregisterAction(s_hDocsAndCommunity);
  xiiActionManager::UnregisterAction(s_hCreateProject);
  xiiActionManager::UnregisterAction(s_hOpenProject);
  xiiActionManager::UnregisterAction(s_hRecentProjects);
  xiiActionManager::UnregisterAction(s_hCloseProject);
  xiiActionManager::UnregisterAction(s_hSettingsCategory);
  xiiActionManager::UnregisterAction(s_hEditorSettingsMenu);
  xiiActionManager::UnregisterAction(s_hProjectSettingsMenu);
  xiiActionManager::UnregisterAction(s_hToolsMenu);
  xiiActionManager::UnregisterAction(s_hToolsCategory);
  xiiActionManager::UnregisterAction(s_hReloadResources);
  xiiActionManager::UnregisterAction(s_hReloadEngine);
  xiiActionManager::UnregisterAction(s_hLaunchFileserve);
  xiiActionManager::UnregisterAction(s_hLaunchInspector);
  xiiActionManager::UnregisterAction(s_hSaveProfiling);
  xiiActionManager::UnregisterAction(s_hOpenVsCode);
  xiiActionManager::UnregisterAction(s_hShortcutEditor);
  xiiActionManager::UnregisterAction(s_hPreferencesDlg);
  xiiActionManager::UnregisterAction(s_hTagsDlg);
  xiiActionManager::UnregisterAction(s_hDataDirectories);
  xiiActionManager::UnregisterAction(s_hWindowConfig);
  xiiActionManager::UnregisterAction(s_hImportAsset);
  xiiActionManager::UnregisterAction(s_hClearAssetCaches);
  xiiActionManager::UnregisterAction(s_hInputConfig);
  xiiActionManager::UnregisterAction(s_hAssetProfiles);
  xiiActionManager::UnregisterAction(s_hCppProjectMenu);
  xiiActionManager::UnregisterAction(s_hSetupCppProject);
  xiiActionManager::UnregisterAction(s_hOpenCppProject);
  xiiActionManager::UnregisterAction(s_hCompileCppProject);
  xiiActionManager::UnregisterAction(s_hExportProject);
  xiiActionManager::UnregisterAction(s_hPluginSelection);
}

void xiiProjectActions::MapActions(const char* szMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hEditorMenu, "", -1000000000.0f);

  pMap->MapAction(s_hDocumentCategory, "Menu.Editor", 1.0f);
  pMap->MapAction(s_hCreateDocument, "Menu.Editor/DocumentCategory", 1.0f);
  pMap->MapAction(s_hOpenDocument, "Menu.Editor/DocumentCategory", 2.0f);
  pMap->MapAction(s_hImportAsset, "Menu.Editor/DocumentCategory", 3.0f);
  pMap->MapAction(s_hRecentDocuments, "Menu.Editor/DocumentCategory", 4.0f);

  pMap->MapAction(s_hProjectCategory, "Menu.Editor", 2.0f);
  pMap->MapAction(s_hOpenDashboard, "Menu.Editor/ProjectCategory", 0.5f);
  // pMap->MapAction(s_hCreateProject, "Menu.Editor/ProjectCategory", 1.0f); // use dashboard
  // pMap->MapAction(s_hOpenProject, "Menu.Editor/ProjectCategory", 2.0f);   // use dashboard
  // pMap->MapAction(s_hRecentProjects, "Menu.Editor/ProjectCategory", 3.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "Menu.Editor/ProjectCategory", 4.0f);
  pMap->MapAction(s_hExportProject, "Menu.Editor/ProjectCategory", 6.0f);
  pMap->MapAction(s_hProjectSettingsMenu, "Menu.Editor/ProjectCategory", 1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "Menu.Editor/ProjectCategory", 5.0f);
  pMap->MapAction(s_hSetupCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 3.0f);

  pMap->MapAction(s_hSettingsCategory, "Menu.Editor", 3.0f);
  pMap->MapAction(s_hEditorSettingsMenu, "Menu.Editor/SettingsCategory", 1.0f);

  pMap->MapAction(s_hToolsMenu, "", 4.5f);
  pMap->MapAction(s_hToolsCategory, "Menu.Tools", 1.0f);
  pMap->MapAction(s_hReloadResources, "Menu.Tools/ToolsCategory", 1.0f);
  pMap->MapAction(s_hReloadEngine, "Menu.Tools/ToolsCategory", 2.0f);
  pMap->MapAction(s_hLaunchFileserve, "Menu.Tools/ToolsCategory", 3.0f);
  pMap->MapAction(s_hLaunchInspector, "Menu.Tools/ToolsCategory", 3.5f);
  pMap->MapAction(s_hSaveProfiling, "Menu.Tools/ToolsCategory", 4.0f);
  pMap->MapAction(s_hOpenVsCode, "Menu.Tools/ToolsCategory", 5.0f);
  pMap->MapAction(s_hClearAssetCaches, "Menu.Tools/ToolsCategory", 6.0f);

  pMap->MapAction(s_hShortcutEditor, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 2.0f);
  pMap->MapAction(s_hPreferencesDlg, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 3.0f);

  pMap->MapAction(s_hPluginSelection, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 0.5f);
  pMap->MapAction(s_hDataDirectories, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 1.0f);
  pMap->MapAction(s_hInputConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 3.0f);
  pMap->MapAction(s_hTagsDlg, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 4.0f);
  pMap->MapAction(s_hWindowConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 5.0f);
  pMap->MapAction(s_hAssetProfiles, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 6.0f);

  pMap->MapAction(s_hDocsAndCommunity, "Menu.Help", 0.0f);
}

////////////////////////////////////////////////////////////////////////
// xiiRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecentDocumentsMenuAction, 0, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;


void xiiRecentDocumentsMenuAction::GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  if (xiiQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  xiiInt32 iMaxDocumentsToAdd = 10;
  for (auto file : xiiQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!xiiOSFile::ExistsFile(file.m_File))
      continue;

    xiiDynamicMenuAction::Item item;

    const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (xiiDocumentManager::FindDocumentTypeFromPath(file.m_File, false, pTypeDesc).Failed())
      continue;

    item.m_UserValue = file.m_File;
    item.m_Icon      = xiiQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon);

    if (xiiToolsProject::IsProjectOpen())
    {
      xiiString sRelativePath;
      if (!xiiToolsProject::GetSingleton()->IsDocumentInAllowedRoot(file.m_File, &sRelativePath))
        continue;

      item.m_sDisplay = sRelativePath;

      out_entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_entries.PushBack(item);
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void xiiRecentDocumentsMenuAction::Execute(const xiiVariant& value)
{
  xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(value.ConvertTo<xiiString>());
}


////////////////////////////////////////////////////////////////////////
// xiiRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecentProjectsMenuAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;


void xiiRecentProjectsMenuAction::GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  if (xiiQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  xiiStringBuilder sTemp;

  for (auto file : xiiQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList())
  {
    if (!xiiOSFile::ExistsFile(file.m_File))
      continue;

    sTemp = file.m_File;
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    xiiDynamicMenuAction::Item item;
    item.m_sDisplay  = sTemp;
    item.m_UserValue = file.m_File;

    out_entries.PushBack(item);
  }
}

void xiiRecentProjectsMenuAction::Execute(const xiiVariant& value)
{
  xiiQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<xiiString>()).IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// xiiProjectAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProjectAction, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiProjectAction::xiiProjectAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiProjectAction::ButtonType::CreateDocument:
      SetIconPath(":/GuiFoundation/Icons/DocumentAdd16.png");
      break;
    case xiiProjectAction::ButtonType::OpenDocument:
      SetIconPath(":/GuiFoundation/Icons/Document16.png");
      break;
    case xiiProjectAction::ButtonType::OpenDashboard:
      SetIconPath(":/GuiFoundation/Icons/Project16.png");
      break;
    case xiiProjectAction::ButtonType::CreateProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectAdd16.png");
      break;
    case xiiProjectAction::ButtonType::OpenProject:
      SetIconPath(":/GuiFoundation/Icons/Project16.png");
      break;
    case xiiProjectAction::ButtonType::CloseProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectClose16.png");
      break;
    case xiiProjectAction::ButtonType::ReloadResources:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources16.png");
      break;
    case xiiProjectAction::ButtonType::LaunchFileserve:
      SetIconPath(":/EditorFramework/Icons/Fileserve16.png");
      break;
    case xiiProjectAction::ButtonType::LaunchInspector:
      SetIconPath(":/EditorFramework/Icons/Inspector16.png");
      break;
    case xiiProjectAction::ButtonType::ReloadEngine:
      SetIconPath(":/GuiFoundation/Icons/ReloadEngine16.png");
      break;
    case xiiProjectAction::ButtonType::DataDirectories:
      SetIconPath(":/EditorFramework/Icons/DataDirectories16.png");
      break;
    case xiiProjectAction::ButtonType::WindowConfig:
      SetIconPath(":/EditorFramework/Icons/WindowConfig16.png");
      break;
    case xiiProjectAction::ButtonType::ImportAsset:
      SetIconPath(":/GuiFoundation/Icons/DocumentImport16.png");
      break;
    case xiiProjectAction::ButtonType::InputConfig:
      SetIconPath(":/EditorFramework/Icons/Input16.png");
      break;
    case xiiProjectAction::ButtonType::PluginSelection:
      SetIconPath(":/EditorFramework/Icons/Plugins16.png");
      break;
    case xiiProjectAction::ButtonType::PreferencesDialog:
      SetIconPath(":/EditorFramework/Icons/StoredSettings16.png");
      break;
    case xiiProjectAction::ButtonType::TagsDialog:
      SetIconPath(":/EditorFramework/Icons/Tag16.png");
      break;
    case xiiProjectAction::ButtonType::ExportProject:
      // TODO: SetIconPath(":/EditorFramework/Icons/Tag16.png");
      break;
    case xiiProjectAction::ButtonType::Shortcuts:
      SetIconPath(":/GuiFoundation/Icons/Shortcuts16.png");
      break;
    case xiiProjectAction::ButtonType::AssetProfiles:
      SetIconPath(":/EditorFramework/Icons/AssetProfiles16.png");
      break;
    case xiiProjectAction::ButtonType::OpenVsCode:
      SetIconPath(":/GuiFoundation/Icons/vscode16.png");
      break;
    case xiiProjectAction::ButtonType::SaveProfiling:
      // no icon
      break;
    case xiiProjectAction::ButtonType::SetupCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case xiiProjectAction::ButtonType::OpenCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case xiiProjectAction::ButtonType::CompileCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case xiiProjectAction::ButtonType::ShowDocsAndCommunity:
      // SetIconPath(":/GuiFoundation/Icons/Project16.png"); // TODO
      break;
    case xiiProjectAction::ButtonType::ClearAssetCaches:
      // SetIconPath(":/GuiFoundation/Icons/Project16.png"); // TODO
      break;
  }

  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    SetEnabled(xiiToolsProject::IsProjectOpen());

    xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(xiiCppProject::ExistsProjectCMakeListsTxt());

    xiiCppProject::s_ChangeEvents.AddEventHandler(xiiMakeDelegate(&xiiProjectAction::CppEventHandler, this));
  }
}

xiiProjectAction::~xiiProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    xiiCppProject::s_ChangeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiProjectAction::CppEventHandler, this));
  }
}

void xiiProjectAction::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(xiiCppProject::ExistsProjectCMakeListsTxt());
  }
  else
  {
    SetEnabled(xiiToolsProject::IsProjectOpen());
  }
}

void xiiProjectAction::CppEventHandler(const xiiCppSettings& e)
{
  SetEnabled(xiiCppProject::ExistsProjectCMakeListsTxt());
}

void xiiProjectAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiProjectAction::ButtonType::CreateDocument:
      xiiQtEditorApp::GetSingleton()->GuiCreateDocument();
      break;

    case xiiProjectAction::ButtonType::OpenDocument:
      xiiQtEditorApp::GetSingleton()->GuiOpenDocument();
      break;

    case xiiProjectAction::ButtonType::OpenDashboard:
      xiiQtEditorApp::GetSingleton()->GuiOpenDashboard();
      break;

    case xiiProjectAction::ButtonType::CreateProject:
      xiiQtEditorApp::GetSingleton()->GuiCreateProject();
      break;

    case xiiProjectAction::ButtonType::OpenProject:
      xiiQtEditorApp::GetSingleton()->GuiOpenProject();
      break;

    case xiiProjectAction::ButtonType::CloseProject:
    {
      if (xiiToolsProject::CanCloseProject())
        xiiQtEditorApp::GetSingleton()->CloseProject();
    }
    break;

    case xiiProjectAction::ButtonType::DataDirectories:
    {
      xiiQtDataDirsDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::WindowConfig:
    {
      xiiQtWindowCfgDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::ImportAsset:
    {
      xiiAssetDocumentGenerator::ImportAssets();
    }
    break;

    case xiiProjectAction::ButtonType::InputConfig:
    {
      xiiQtInputConfigDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        xiiToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case xiiProjectAction::ButtonType::PluginSelection:
    {
      xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles();

      xiiQtPluginSelectionDlg dlg(&xiiQtEditorApp::GetSingleton()->GetPluginBundles());
      dlg.exec();

      xiiToolsProject::SaveProjectState();
    }
    break;

    case xiiProjectAction::ButtonType::PreferencesDialog:
    {
      xiiQtPreferencesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // save modified preferences right away
        xiiToolsProject::SaveProjectState();

        xiiToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case xiiProjectAction::ButtonType::TagsDialog:
    {
      xiiQtTagsDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        xiiToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case xiiProjectAction::ButtonType::ExportProject:
    {
      xiiQtExportProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::ClearAssetCaches:
    {
      auto res = xiiQtUiServices::GetSingleton()->MessageBoxQuestion("Delete ALL cached asset files?\n\n* 'Yes All' deletes everything and takes a long time to re-process. This is rarely needed.\n* 'No All' only deletes assets that are likely to make problems.", QMessageBox::StandardButton::YesAll | QMessageBox::StandardButton::NoAll | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

      if (res == QMessageBox::StandardButton::Cancel)
        break;

      if (res == QMessageBox::StandardButton::YesAll)
        xiiAssetCurator::GetSingleton()->ClearAssetCaches(xiiAssetDocumentManager::Perfect);
      else
        xiiAssetCurator::GetSingleton()->ClearAssetCaches(xiiAssetDocumentManager::Unknown);
    }
    break;

    case xiiProjectAction::ButtonType::Shortcuts:
    {
      xiiQtShortcutEditorDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::ReloadResources:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", xiiTime::Seconds(5));

      xiiSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      msg.m_sPayload  = "ReloadAllResources";
      xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      xiiEditorAppEvent e;
      e.m_Type = xiiEditorAppEvent::Type::ReloadResources;
      xiiQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }

      xiiTranslator::ReloadAllTranslators();
    }
    break;

    case xiiProjectAction::ButtonType::LaunchFileserve:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", xiiTime::Seconds(5));

      xiiQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::LaunchInspector:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching xiiInspector...", xiiTime::Seconds(5));

      xiiQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case xiiProjectAction::ButtonType::ReloadEngine:
    {
      xiiEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
    }
    break;

    case xiiProjectAction::ButtonType::SaveProfiling:
    {
      const char* szEditorProfilingFile = ":appdata/profilingEditor.json";
      {
        // Start capturing profiling data on engine process
        xiiSimpleConfigMsgToEngine msg;
        msg.m_sWhatToDo = "SaveProfiling";
        msg.m_sPayload  = ":appdata/profilingEngine.json";
        xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      }
      {
        // Capture profiling data on editor process
        xiiFileWriter fileWriter;
        if (fileWriter.Open(szEditorProfilingFile) == XII_SUCCESS)
        {
          xiiProfilingSystem::ProfilingData profilingData;
          xiiProfilingSystem::Capture(profilingData);
          // Set sort index to -1 so that the editor is always on top when opening the trace.
          profilingData.m_uiProcessSortIndex = -1;
          if (profilingData.Write(fileWriter).Failed())
          {
            xiiLog::Error("Failed to write editor profiling capture: {}.", szEditorProfilingFile);
            return;
          }

          xiiLog::Info("Editor profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          xiiLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
      }
      xiiStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](xiiProcessMessage* pMsg) -> bool {
          auto pSimpleCfg      = static_cast<xiiSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        xiiProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForMessage(xiiGetStaticRTTI<xiiSaveProfilingResponseToEditor>(), xiiTime::Seconds(15), &cb).Failed())
        {
          xiiLog::Error("Timeout while waiting for engine process to create profiling capture. Captures will not be merged.");
          return;
        }
        if (sEngineProfilingFile.IsEmpty())
        {
          xiiLog::Error("Engine process failed to create profiling file.");
          return;
        }
      }

      // Merge editor and engine profiling files by simply merging the arrays inside
      {
        xiiString sEngineProfilingJson;
        {
          xiiFileReader reader;
          if (reader.Open(sEngineProfilingFile).Failed())
          {
            xiiLog::Error("Failed to read engine profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEngineProfilingJson.ReadAll(reader);
        }
        xiiString sEditorProfilingJson;
        {
          xiiFileReader reader;
          if (reader.Open(szEditorProfilingFile).Failed())
          {
            xiiLog::Error("Failed to read editor profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEditorProfilingJson.ReadAll(reader);
        }

        xiiStringBuilder sMergedProfilingJson;
        {
          // Just glue the array together
          sMergedProfilingJson.Reserve(sEngineProfilingJson.GetElementCount() + 1 + sEditorProfilingJson.GetElementCount());
          const char* szEndArray = sEngineProfilingJson.FindLastSubString("]");
          sMergedProfilingJson.Append(xiiStringView(sEngineProfilingJson.GetData(), szEndArray - sEngineProfilingJson.GetData()));
          sMergedProfilingJson.Append(",");
          const char* szStartArray = sEditorProfilingJson.FindSubString("[") + 1;
          sMergedProfilingJson.Append(xiiStringView(szStartArray, sEditorProfilingJson.GetElementCount() - (szStartArray - sEditorProfilingJson.GetData())));
        }
        xiiStringBuilder  sMergedFile;
        const xiiDateTime dt = xiiTimestamp::CurrentTimestamp();
        sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), xiiArgU(dt.GetMonth(), 2, true), xiiArgU(dt.GetDay(), 2, true), xiiArgU(dt.GetHour(), 2, true), xiiArgU(dt.GetMinute(), 2, true), xiiArgU(dt.GetSecond(), 2, true), xiiArgU(dt.GetMicroseconds() / 1000, 3, true));
        xiiFileWriter fileWriter;
        if (fileWriter.Open(sMergedFile).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
        {
          xiiLog::Error("Failed to write merged profiling capture: {}.", sMergedFile);
          return;
        }
        xiiLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData()), xiiTime::Seconds(5.0));
      }
    }
    break;

    case xiiProjectAction::ButtonType::OpenVsCode:
    {
      QStringList args;

      for (const auto& dd : xiiQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
      {
        xiiStringBuilder path;
        xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

        args.append(QString::fromUtf8(path, path.GetElementCount()));
      }

      const xiiStatus res = xiiQtUiServices::OpenInVsCode(args);

      xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to open VS Code");
    }
    break;

    case xiiProjectAction::ButtonType::AssetProfiles:
    {
      xiiQtAssetProfilesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // we need to force the asset status reevaluation because when the profile settings have changed,
        // we need to figure out which assets are now out of date
        xiiAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(dlg.m_uiActiveConfig, true);

        // makes the scene re-select the current objects, which updates which enum values are shown in the property grid
        xiiToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case xiiProjectAction::ButtonType::SetupCppProject:
    {
      xiiQtCppProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::OpenCppProject:
    {
      xiiCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (xiiCppProject::ExistsProjectCMakeListsTxt())
      {
        if (xiiCppProject::RunCMakeIfNecessary(cpp).Failed())
        {
          xiiQtUiServices::GetSingleton()->MessageBoxWarning("Generating the C++ solution failed.");
        }
        else if (!xiiQtUiServices::OpenFileInDefaultProgram(xiiCppProject::GetSolutionPath(cpp)))
        {
          xiiQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
        }
      }
      else
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, opening a solution is not possible.");
      }
    }
    break;

    case xiiProjectAction::ButtonType::CompileCppProject:
    {
      xiiCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (xiiCppProject::ExistsProjectCMakeListsTxt())
      {
        if (xiiCppProject::BuildCodeIfNecessary(cpp).Succeeded())
        {
          xiiQtUiServices::GetSingleton()->MessageBoxInformation("Successfully compiled the C++ code.");
        }
        else
        {
          xiiQtUiServices::GetSingleton()->MessageBoxWarning("Compiling the code failed. See log for details.");
        }
      }
      else
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not necessary.");
      }
    }
    break;

    case xiiProjectAction::ButtonType::ShowDocsAndCommunity:
      xiiQtEditorApp::GetSingleton()->GuiOpenDocsAndCommunity();
      break;
  }
}
