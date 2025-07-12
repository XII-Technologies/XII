#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/CodeGen/CppProject.h>
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
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

xiiActionDescriptorHandle xiiProjectActions::s_hCatProjectGeneral;
xiiActionDescriptorHandle xiiProjectActions::s_hCatProjectAssets;
xiiActionDescriptorHandle xiiProjectActions::s_hCatProjectConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hCatProjectExternal;

xiiActionDescriptorHandle xiiProjectActions::s_hCatFilesGeneral;
xiiActionDescriptorHandle xiiProjectActions::s_hCatFileCommon;
xiiActionDescriptorHandle xiiProjectActions::s_hCatFileSpecial;
xiiActionDescriptorHandle xiiProjectActions::s_hCatAssetDoc;

xiiActionDescriptorHandle xiiProjectActions::s_hCreateDocument;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenDocument;
xiiActionDescriptorHandle xiiProjectActions::s_hRecentDocuments;

xiiActionDescriptorHandle xiiProjectActions::s_hOpenDashboard;
xiiActionDescriptorHandle xiiProjectActions::s_hCreateProject;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenProject;
xiiActionDescriptorHandle xiiProjectActions::s_hRecentProjects;
xiiActionDescriptorHandle xiiProjectActions::s_hCloseProject;
xiiActionDescriptorHandle xiiProjectActions::s_hDocsAndCommunity;

xiiActionDescriptorHandle xiiProjectActions::s_hCatProjectSettings;
xiiActionDescriptorHandle xiiProjectActions::s_hCatPluginSettings;
xiiActionDescriptorHandle xiiProjectActions::s_hShortcutEditor;
xiiActionDescriptorHandle xiiProjectActions::s_hDataDirectories;
xiiActionDescriptorHandle xiiProjectActions::s_hWindowConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hInputConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hPreferencesDlg;
xiiActionDescriptorHandle xiiProjectActions::s_hTagsConfig;
xiiActionDescriptorHandle xiiProjectActions::s_hImportAsset;
xiiActionDescriptorHandle xiiProjectActions::s_hAssetProfiles;
xiiActionDescriptorHandle xiiProjectActions::s_hExportProject;
xiiActionDescriptorHandle xiiProjectActions::s_hPluginSelection;
xiiActionDescriptorHandle xiiProjectActions::s_hClearAssetCaches;

xiiActionDescriptorHandle xiiProjectActions::s_hCatToolsExternal;
xiiActionDescriptorHandle xiiProjectActions::s_hCatToolsEditor;
xiiActionDescriptorHandle xiiProjectActions::s_hCatToolsDocument;
xiiActionDescriptorHandle xiiProjectActions::s_hCatEditorSettings;
xiiActionDescriptorHandle xiiProjectActions::s_hReloadResources;
xiiActionDescriptorHandle xiiProjectActions::s_hReloadEngine;
xiiActionDescriptorHandle xiiProjectActions::s_hLaunchFileserve;
xiiActionDescriptorHandle xiiProjectActions::s_hLaunchInspector;
xiiActionDescriptorHandle xiiProjectActions::s_hLaunchTracy;
xiiActionDescriptorHandle xiiProjectActions::s_hSaveProfiling;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenVsCode;

xiiActionDescriptorHandle xiiProjectActions::s_hCppProjectMenu;
xiiActionDescriptorHandle xiiProjectActions::s_hSetupCppProject;
xiiActionDescriptorHandle xiiProjectActions::s_hOpenCppProject;
xiiActionDescriptorHandle xiiProjectActions::s_hCompileCppProject;
xiiActionDescriptorHandle xiiProjectActions::s_hRegenerateCppSolution;

void xiiProjectActions::RegisterActions()
{
  s_hCatProjectGeneral  = XII_REGISTER_CATEGORY("G.Project.General");
  s_hCatProjectAssets   = XII_REGISTER_CATEGORY("G.Project.Assets");
  s_hCatProjectExternal = XII_REGISTER_CATEGORY("G.Project.External");
  s_hCatProjectConfig   = XII_REGISTER_CATEGORY("G.Project.Config");
  s_hCatEditorSettings  = XII_REGISTER_CATEGORY("G.Editor.Settings");

  s_hCatFilesGeneral = XII_REGISTER_CATEGORY("G.Files.General");
  s_hCatFileCommon   = XII_REGISTER_CATEGORY("G.File.Common");
  s_hCatFileSpecial  = XII_REGISTER_CATEGORY("G.File.Special");
  s_hCatAssetDoc     = XII_REGISTER_CATEGORY("G.AssetDoc");

  s_hOpenDashboard = XII_REGISTER_ACTION_1("Editor.OpenDashboard", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::OpenDashboard);

  s_hCreateProject = XII_REGISTER_ACTION_1("Project.Create", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CreateProject);

  s_hOpenProject = XII_REGISTER_ACTION_1("Project.Open", xiiActionScope::Global, "Project", "Ctrl+Shift+D", xiiProjectAction, xiiProjectAction::ButtonType::OpenProject);

  s_hRecentProjects = XII_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", xiiRecentProjectsMenuAction, "");
  s_hCloseProject   = XII_REGISTER_ACTION_1("Project.Close", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CloseProject);

  s_hImportAsset      = XII_REGISTER_ACTION_1("Project.ImportAsset", xiiActionScope::Global, "Project", "Ctrl+I", xiiProjectAction, xiiProjectAction::ButtonType::ImportAsset);
  s_hClearAssetCaches = XII_REGISTER_ACTION_1("Project.ClearAssetCaches", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::ClearAssetCaches);

  s_hExportProject = XII_REGISTER_ACTION_1("Project.ExportProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::ExportProject);

  s_hCppProjectMenu = XII_REGISTER_MENU("G.Project.Cpp");
  {
    s_hSetupCppProject       = XII_REGISTER_ACTION_1("Project.SetupCppProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::SetupCppProject);
    s_hOpenCppProject        = XII_REGISTER_ACTION_1("Project.OpenCppProject", xiiActionScope::Global, "Project", "Ctrl+Shift+O", xiiProjectAction, xiiProjectAction::ButtonType::OpenCppProject);
    s_hCompileCppProject     = XII_REGISTER_ACTION_1("Project.CompileCppProject", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::CompileCppProject);
    s_hRegenerateCppSolution = XII_REGISTER_ACTION_1("Project.RegenerateCppSolution", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::RegenerateCppSolution);
  }

  s_hCatProjectSettings = XII_REGISTER_MENU("G.Project.Settings");

  s_hPluginSelection = XII_REGISTER_ACTION_1("Project.PluginSelection", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::PluginSelection);
  s_hDataDirectories = XII_REGISTER_ACTION_1("Project.DataDirectories", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::DataDirectories);
  s_hTagsConfig      = XII_REGISTER_ACTION_1("Engine.Tags", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::TagsDialog);
  s_hInputConfig     = XII_REGISTER_ACTION_1("Project.InputConfig", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::InputConfig);
  s_hWindowConfig    = XII_REGISTER_ACTION_1("Project.WindowConfig", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::WindowConfig);
  s_hAssetProfiles   = XII_REGISTER_ACTION_1("Project.AssetProfiles", xiiActionScope::Global, "Project", "", xiiProjectAction, xiiProjectAction::ButtonType::AssetProfiles);

  s_hCatPluginSettings = XII_REGISTER_MENU("G.Plugins.Settings");

  //////////////////////////////////////////////////////////////////////////

  s_hCreateDocument  = XII_REGISTER_ACTION_1("Document.Create", xiiActionScope::Global, "Project", "Ctrl+N", xiiProjectAction, xiiProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument    = XII_REGISTER_ACTION_1("Document.Open", xiiActionScope::Global, "Project", "Ctrl+O", xiiProjectAction, xiiProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = XII_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", xiiRecentDocumentsMenuAction, "");

  s_hShortcutEditor = XII_REGISTER_ACTION_1("Editor.Shortcuts", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg = XII_REGISTER_ACTION_1("Editor.Preferences", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::PreferencesDialog);

  s_hCatToolsExternal = XII_REGISTER_CATEGORY("G.Tools.External");
  s_hCatToolsEditor   = XII_REGISTER_CATEGORY("G.Tools.Editor");
  s_hCatToolsDocument = XII_REGISTER_CATEGORY("G.Tools.Document");

  s_hReloadResources = XII_REGISTER_ACTION_1("Engine.ReloadResources", xiiActionScope::Global, "Engine", "F4", xiiProjectAction, xiiProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine    = XII_REGISTER_ACTION_1("Engine.ReloadEngine", xiiActionScope::Global, "Engine", "Ctrl+Shift+F4", xiiProjectAction, xiiProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = XII_REGISTER_ACTION_1("Editor.LaunchFileserve", xiiActionScope::Global, "Engine", "", xiiProjectAction, xiiProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = XII_REGISTER_ACTION_1("Editor.LaunchInspector", xiiActionScope::Global, "Engine", "", xiiProjectAction, xiiProjectAction::ButtonType::LaunchInspector);
  s_hLaunchTracy     = XII_REGISTER_ACTION_1("Editor.LaunchTracy", xiiActionScope::Global, "Engine", "", xiiProjectAction, xiiProjectAction::ButtonType::LaunchTracy);
  s_hSaveProfiling   = XII_REGISTER_ACTION_1("Editor.SaveProfiling", xiiActionScope::Global, "Engine", "Ctrl+Alt+P", xiiProjectAction, xiiProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode      = XII_REGISTER_ACTION_1("Editor.OpenVsCode", xiiActionScope::Global, "Project", "Ctrl+Alt+O", xiiProjectAction, xiiProjectAction::ButtonType::OpenVsCode);

  s_hDocsAndCommunity = XII_REGISTER_ACTION_1("Editor.DocsAndCommunity", xiiActionScope::Global, "Editor", "", xiiProjectAction, xiiProjectAction::ButtonType::ShowDocsAndCommunity);
}

void xiiProjectActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCatProjectGeneral);
  xiiActionManager::UnregisterAction(s_hCatProjectAssets);
  xiiActionManager::UnregisterAction(s_hCatProjectConfig);
  xiiActionManager::UnregisterAction(s_hCatProjectExternal);

  xiiActionManager::UnregisterAction(s_hCatFilesGeneral);
  xiiActionManager::UnregisterAction(s_hCatFileCommon);
  xiiActionManager::UnregisterAction(s_hCatFileSpecial);
  xiiActionManager::UnregisterAction(s_hCatAssetDoc);

  xiiActionManager::UnregisterAction(s_hCreateDocument);
  xiiActionManager::UnregisterAction(s_hOpenDocument);
  xiiActionManager::UnregisterAction(s_hRecentDocuments);
  xiiActionManager::UnregisterAction(s_hOpenDashboard);
  xiiActionManager::UnregisterAction(s_hDocsAndCommunity);
  xiiActionManager::UnregisterAction(s_hCreateProject);
  xiiActionManager::UnregisterAction(s_hOpenProject);
  xiiActionManager::UnregisterAction(s_hRecentProjects);
  xiiActionManager::UnregisterAction(s_hCloseProject);
  xiiActionManager::UnregisterAction(s_hCatProjectSettings);
  xiiActionManager::UnregisterAction(s_hCatPluginSettings);
  xiiActionManager::UnregisterAction(s_hCatToolsExternal);
  xiiActionManager::UnregisterAction(s_hCatToolsEditor);
  xiiActionManager::UnregisterAction(s_hCatToolsDocument);
  xiiActionManager::UnregisterAction(s_hCatEditorSettings);
  xiiActionManager::UnregisterAction(s_hReloadResources);
  xiiActionManager::UnregisterAction(s_hReloadEngine);
  xiiActionManager::UnregisterAction(s_hLaunchFileserve);
  xiiActionManager::UnregisterAction(s_hLaunchInspector);
  xiiActionManager::UnregisterAction(s_hLaunchTracy);
  xiiActionManager::UnregisterAction(s_hSaveProfiling);
  xiiActionManager::UnregisterAction(s_hOpenVsCode);
  xiiActionManager::UnregisterAction(s_hShortcutEditor);
  xiiActionManager::UnregisterAction(s_hPreferencesDlg);
  xiiActionManager::UnregisterAction(s_hTagsConfig);
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
  xiiActionManager::UnregisterAction(s_hRegenerateCppSolution);
  xiiActionManager::UnregisterAction(s_hExportProject);
  xiiActionManager::UnregisterAction(s_hPluginSelection);
}

void xiiProjectActions::MapActions(xiiStringView sMapping, const xiiBitflags<xiiStandardMenuTypes> menus)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  // Add categories
  pMap->MapAction(s_hCatProjectGeneral, "G.Project", 1.0f);
  pMap->MapAction(s_hCatProjectConfig, "G.Project", 2.0f);
  pMap->MapAction(s_hCatProjectAssets, "G.Project", 4.0f);
  pMap->MapAction(s_hCatProjectExternal, "G.Project", 5.0f);

  pMap->MapAction(s_hCatToolsExternal, "G.Tools", 1.0f);
  pMap->MapAction(s_hCatToolsEditor, "G.Tools", 2.0f);
  pMap->MapAction(s_hCatToolsDocument, "G.Tools", 3.0f);
  pMap->MapAction(s_hCatEditorSettings, "G.Tools", 1000.0f);

  pMap->MapAction(s_hCatProjectSettings, "G.Project.Config", 1.0f);
  pMap->MapAction(s_hCatPluginSettings, "G.Project.Config", 1.0f);

  if (menus.IsSet(xiiStandardMenuTypes::File))
  {
    pMap->MapAction(s_hCatFilesGeneral, "G.File", 1.0f);
    pMap->MapAction(s_hCatFileCommon, "G.File", 2.0f);
    pMap->MapAction(s_hCatAssetDoc, "G.File", 3.0f);
    pMap->MapAction(s_hCatFileSpecial, "G.File", 4.0f);
  }

  // Add actions
  // pMap->MapAction(s_hOpenDashboard, "G.Project.General", 1.0f);
  pMap->MapAction(s_hOpenProject, "G.Project.General", 2.0f);   // use dashboard
  pMap->MapAction(s_hCreateProject, "G.Project.General", 3.0f); // use dashboard
  // pMap->MapAction(s_hRecentProjects, "G.Project.General", 4.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "G.Project.General", 5.0f);

  pMap->MapAction(s_hImportAsset, "G.Project.Assets", 1.0f);
  pMap->MapAction(s_hClearAssetCaches, "G.Project.Assets", 5.0f);

  pMap->MapAction(s_hDataDirectories, "G.Project.Settings", 1.0f);
  pMap->MapAction(s_hInputConfig, "G.Project.Settings", 2.0f);
  pMap->MapAction(s_hWindowConfig, "G.Project.Settings", 3.0f);
  pMap->MapAction(s_hTagsConfig, "G.Project.Settings", 4.0f);
  pMap->MapAction(s_hAssetProfiles, "G.Project.Settings", 5.0f);

  pMap->MapAction(s_hPluginSelection, "G.Plugins.Settings", -1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "G.Project.External", 1.0f);
  pMap->MapAction(s_hSetupCppProject, "G.Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "G.Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "G.Project.Cpp", 3.0f);
  pMap->MapAction(s_hRegenerateCppSolution, "G.Project.Cpp", 4.0f);
  pMap->MapAction(s_hExportProject, "G.Project.External", 10.0f);

  pMap->MapAction(s_hOpenVsCode, "G.Tools.External", 1.0f);
  pMap->MapAction(s_hLaunchInspector, "G.Tools.External", 2.0f);
  pMap->MapAction(s_hLaunchTracy, "G.Tools.External", 3.0f);
  pMap->MapAction(s_hLaunchFileserve, "G.Tools.External", 4.0f);

  pMap->MapAction(s_hReloadResources, "G.Tools.Editor", 1.0f);
  pMap->MapAction(s_hReloadEngine, "G.Tools.Editor", 2.0f);
  pMap->MapAction(s_hSaveProfiling, "G.Tools.Editor", 3.0f);

  pMap->MapAction(s_hShortcutEditor, "G.Editor.Settings", 1.0f);
  pMap->MapAction(s_hPreferencesDlg, "G.Editor.Settings", 2.0f);

  if (menus.IsSet(xiiStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hDocsAndCommunity, "G.Help", 0.0f);
  }

  if (menus.IsSet(xiiStandardMenuTypes::File))
  {
    pMap->MapAction(s_hCreateDocument, "G.Files.General", 1.0f);
    pMap->MapAction(s_hOpenDocument, "G.Files.General", 2.0f);
    pMap->MapAction(s_hRecentDocuments, "G.Files.General", 3.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecentDocumentsMenuAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRecentDocumentsMenuAction::GetEntries(xiiDynamicArray<Item>& out_entries)
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
    item.m_Icon      = xiiQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon, xiiColorScheme::GetCategoryColor(pTypeDesc->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::MenuEntryIcon));

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRecentProjectsMenuAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiRecentProjectsMenuAction::GetEntries(xiiDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProjectAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiProjectAction::xiiProjectAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiProjectAction::ButtonType::CreateDocument:
      SetIconPath(":/GuiFoundation/Icons/DocumentAdd.svg");
      break;
    case xiiProjectAction::ButtonType::OpenDocument:
      SetIconPath(":/GuiFoundation/Icons/Document.svg");
      break;
    case xiiProjectAction::ButtonType::OpenDashboard:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case xiiProjectAction::ButtonType::CreateProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectAdd.svg");
      break;
    case xiiProjectAction::ButtonType::OpenProject:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case xiiProjectAction::ButtonType::CloseProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectClose.svg");
      break;
    case xiiProjectAction::ButtonType::ReloadResources:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources.svg");
      break;
    case xiiProjectAction::ButtonType::LaunchFileserve:
      SetIconPath(":/EditorFramework/Icons/Fileserve.svg");
      break;
    case xiiProjectAction::ButtonType::LaunchInspector:
      SetIconPath(":/EditorFramework/Icons/Inspector.svg");
      break;
    case xiiProjectAction::ButtonType::LaunchTracy:
      SetIconPath(":/EditorFramework/Icons/Tracy.svg");
      break;
    case xiiProjectAction::ButtonType::ReloadEngine:
      SetIconPath(":/GuiFoundation/Icons/ReloadEngine.svg");
      break;
    case xiiProjectAction::ButtonType::DataDirectories:
      SetIconPath(":/EditorFramework/Icons/DataDirectory.svg");
      break;
    case xiiProjectAction::ButtonType::WindowConfig:
      SetIconPath(":/EditorFramework/Icons/WindowConfig.svg");
      break;
    case xiiProjectAction::ButtonType::ImportAsset:
      SetIconPath(":/GuiFoundation/Icons/Import.svg");
      break;
    case xiiProjectAction::ButtonType::InputConfig:
      SetIconPath(":/EditorFramework/Icons/Input.svg");
      break;
    case xiiProjectAction::ButtonType::PluginSelection:
      SetIconPath(":/EditorFramework/Icons/Plugins.svg");
      break;
    case xiiProjectAction::ButtonType::PreferencesDialog:
      SetIconPath(":/EditorFramework/Icons/StoredSettings.svg");
      break;
    case xiiProjectAction::ButtonType::TagsDialog:
      SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case xiiProjectAction::ButtonType::ExportProject:
      // TODO: SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case xiiProjectAction::ButtonType::Shortcuts:
      SetIconPath(":/GuiFoundation/Icons/Shortcuts.svg");
      break;
    case xiiProjectAction::ButtonType::AssetProfiles:
      SetIconPath(":/EditorFramework/Icons/AssetProfile.svg");
      break;
    case xiiProjectAction::ButtonType::OpenVsCode:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
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
    case xiiProjectAction::ButtonType::RegenerateCppSolution:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case xiiProjectAction::ButtonType::ShowDocsAndCommunity:
      // SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
      break;
    case xiiProjectAction::ButtonType::ClearAssetCaches:
      // SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
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
      m_ButtonType == ButtonType::LaunchTracy ||
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
      m_ButtonType == ButtonType::LaunchTracy ||
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

  if (m_ButtonType == ButtonType::OpenCppProject || m_ButtonType == ButtonType::CompileCppProject)
  {
    xiiCppProject::s_ChangeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiProjectAction::CppEventHandler, this));
  }
}

void xiiProjectAction::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject || m_ButtonType == ButtonType::CompileCppProject)
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
  if (m_ButtonType == ButtonType::OpenCppProject || m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(xiiCppProject::ExistsProjectCMakeListsTxt());
  }
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
      xiiQtEditorApp::GetSingleton()->GuiOpenDashboard();
      // xiiQtEditorApp::GetSingleton()->GuiOpenProject();
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
      xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(xiiOSFile::GetApplicationDirectory());

      xiiCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(xiiCppProject::GetPluginSourceDir(cppSettings));
      }

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
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", xiiTime::MakeFromSeconds(5));

      xiiSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      msg.m_sPayload  = "ReloadAllResources";
      xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      xiiEditorAppEvent e;
      e.m_Type = xiiEditorAppEvent::Type::ReloadResources;
      xiiQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

      // keep this here to make live color palette editing available, when needed
      if (false)
      {
        QTimer::singleShot(1, [this]() { xiiQtEditorApp::GetSingleton()->SetStyleSheet(); });
        QTimer::singleShot(500, [this]() { xiiQtEditorApp::GetSingleton()->SetStyleSheet(); });
      }

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }

      xiiTranslator::ReloadAllTranslators();
    }
    break;

    case xiiProjectAction::ButtonType::LaunchFileserve:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", xiiTime::MakeFromSeconds(5));

      xiiQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case xiiProjectAction::ButtonType::LaunchInspector:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching xiiInspector...", xiiTime::MakeFromSeconds(5));

      xiiQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case xiiProjectAction::ButtonType::LaunchTracy:
    {
      xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching Tracy...", xiiTime::MakeFromSeconds(5));

      xiiQtEditorApp::GetSingleton()->RunTracy();
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
      if (xiiProfilingUtils::SaveProfilingCapture(szEditorProfilingFile).Failed())
        return;

      xiiStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](xiiProcessMessage* pMsg) -> bool {
          auto pSimpleCfg      = static_cast<xiiSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        xiiProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForMessage(xiiGetStaticRTTI<xiiSaveProfilingResponseToEditor>(), xiiTime::MakeFromSeconds(15), &cb).Failed())
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

      xiiStringBuilder  sMergedFile;
      const xiiDateTime dt = xiiDateTime::MakeFromTimestamp(xiiTimestamp::CurrentTimestamp());
      sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), xiiArgU(dt.GetMonth(), 2, true), xiiArgU(dt.GetDay(), 2, true), xiiArgU(dt.GetHour(), 2, true), xiiArgU(dt.GetMinute(), 2, true), xiiArgU(dt.GetSecond(), 2, true), xiiArgU(dt.GetMicroseconds() / 1000, 3, true));

      xiiStringBuilder sAbsPath;
      if (xiiProfilingUtils::MergeProfilingCaptures(sEngineProfilingFile, szEditorProfilingFile, sMergedFile).Succeeded() && xiiFileSystem::ResolvePath(sMergedFile, &sAbsPath, nullptr).Succeeded())
      {
        xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Merged profiling capture saved to '{0}'.", sAbsPath), xiiTime::MakeFromSeconds(5.0));
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
        else
        {
          if (auto status = xiiCppProject::OpenSolution(cpp); status.Failed())
          {
            xiiQtUiServices::GetSingleton()->MessageBoxWarning(status.GetMessageString().GetView());
          }
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
        xiiQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not possible (or necessary).");
      }
    }
    break;

    case xiiProjectAction::ButtonType::RegenerateCppSolution:
    {
      xiiCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (!xiiCppProject::ExistsProjectCMakeListsTxt() || !xiiCppProject::ExistsSolution(cpp))
      {
        xiiQtCppProjectDlg dlg(nullptr);
        dlg.exec();
      }
      else
      {
        if (xiiCppProject::RunCMake(cpp).Succeeded())
        {
          xiiQtUiServices::GetSingleton()->MessageBoxInformation("Successfully regenerated the C++ solution.");
        }
        else
        {
          xiiQtUiServices::GetSingleton()->MessageBoxWarning("Regenerating the solution failed. See log for details.");
        }
      }
    }
    break;

    case xiiProjectAction::ButtonType::ShowDocsAndCommunity:
      xiiQtEditorApp::GetSingleton()->GuiOpenDocsAndCommunity();
      break;
  }
}
