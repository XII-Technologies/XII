#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiCppSettings;

///
class XII_EDITORFRAMEWORK_DLL xiiProjectActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping, const xiiBitflags<xiiStandardMenuTypes> menus = xiiStandardMenuTypes::Default);

  static xiiActionDescriptorHandle s_hCatProjectGeneral;
  static xiiActionDescriptorHandle s_hCatProjectAssets;
  static xiiActionDescriptorHandle s_hCatProjectConfig;
  static xiiActionDescriptorHandle s_hCatProjectExternal;

  static xiiActionDescriptorHandle s_hCatFilesGeneral;
  static xiiActionDescriptorHandle s_hCatFileCommon;
  static xiiActionDescriptorHandle s_hCatFileSpecial;
  static xiiActionDescriptorHandle s_hCatAssetDoc;

  static xiiActionDescriptorHandle s_hCreateDocument;
  static xiiActionDescriptorHandle s_hOpenDocument;
  static xiiActionDescriptorHandle s_hRecentDocuments;

  static xiiActionDescriptorHandle s_hOpenDashboard;
  static xiiActionDescriptorHandle s_hCreateProject;
  static xiiActionDescriptorHandle s_hOpenProject;
  static xiiActionDescriptorHandle s_hRecentProjects;
  static xiiActionDescriptorHandle s_hCloseProject;

  static xiiActionDescriptorHandle s_hDocsAndCommunity;

  static xiiActionDescriptorHandle s_hCatProjectSettings;
  static xiiActionDescriptorHandle s_hCatPluginSettings;
  static xiiActionDescriptorHandle s_hShortcutEditor;
  static xiiActionDescriptorHandle s_hDataDirectories;
  static xiiActionDescriptorHandle s_hWindowConfig;
  static xiiActionDescriptorHandle s_hInputConfig;
  static xiiActionDescriptorHandle s_hPreferencesDlg;
  static xiiActionDescriptorHandle s_hTagsConfig;
  static xiiActionDescriptorHandle s_hAssetProfiles;
  static xiiActionDescriptorHandle s_hExportProject;
  static xiiActionDescriptorHandle s_hPluginSelection;

  static xiiActionDescriptorHandle s_hCatToolsExternal;
  static xiiActionDescriptorHandle s_hCatToolsEditor;
  static xiiActionDescriptorHandle s_hCatToolsDocument;
  static xiiActionDescriptorHandle s_hCatEditorSettings;
  static xiiActionDescriptorHandle s_hReloadResources;
  static xiiActionDescriptorHandle s_hReloadEngine;
  static xiiActionDescriptorHandle s_hLaunchFileserve;
  static xiiActionDescriptorHandle s_hLaunchInspector;
  static xiiActionDescriptorHandle s_hLaunchTracy;
  static xiiActionDescriptorHandle s_hSaveProfiling;
  static xiiActionDescriptorHandle s_hOpenVsCode;
  static xiiActionDescriptorHandle s_hImportAsset;
  static xiiActionDescriptorHandle s_hClearAssetCaches;

  static xiiActionDescriptorHandle s_hCppProjectMenu;
  static xiiActionDescriptorHandle s_hSetupCppProject;
  static xiiActionDescriptorHandle s_hOpenCppProject;
  static xiiActionDescriptorHandle s_hCompileCppProject;
  static xiiActionDescriptorHandle s_hRegenerateCppSolution;
};

///
class XII_EDITORFRAMEWORK_DLL xiiRecentDocumentsMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRecentDocumentsMenuAction, xiiDynamicMenuAction);

public:
  xiiRecentDocumentsMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override;
  virtual void Execute(const xiiVariant& value) override;
};

///
class XII_EDITORFRAMEWORK_DLL xiiRecentProjectsMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRecentProjectsMenuAction, xiiDynamicMenuAction);

public:
  xiiRecentProjectsMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override;
  virtual void Execute(const xiiVariant& value) override;
};

///
class XII_EDITORFRAMEWORK_DLL xiiProjectAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProjectAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    CreateDocument,
    OpenDocument,
    OpenDashboard,
    CreateProject,
    OpenProject,
    CloseProject,
    ReloadResources,
    ReloadEngine,
    LaunchFileserve,
    LaunchInspector,
    LaunchTracy,
    SaveProfiling,
    OpenVsCode,
    Shortcuts,
    DataDirectories,
    WindowConfig,
    InputConfig,
    PreferencesDialog,
    TagsDialog,
    ImportAsset,
    AssetProfiles,
    SetupCppProject,
    OpenCppProject,
    CompileCppProject,
    RegenerateCppSolution,
    ShowDocsAndCommunity,
    ExportProject,
    PluginSelection,
    ClearAssetCaches,
  };

  xiiProjectAction(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiProjectAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void ProjectEventHandler(const xiiToolsProjectEvent& e);
  void CppEventHandler(const xiiCppSettings& e);

  ButtonType m_ButtonType;
};
