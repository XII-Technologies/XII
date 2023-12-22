#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

#include <QApplication>

class QMainWindow;
class QWidget;
class xiiProgress;
class xiiQtProgressbar;
class xiiQtEditorApp;
template <typename T>
class QList;
using QStringList = QList<QString>;
class xiiTranslatorFromFiles;
class xiiDynamicStringEnum;
class QSplashScreen;
class xiiQtVersionChecker;

struct XII_EDITORFRAMEWORK_DLL xiiEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom
                                ///< dependencies at the right moment.
    ReloadResources,            ///< Sent when 'ReloadResources' has been triggered (and a message was sent to the engine)
    EditorStarted,              ///< Editor has finished all initialization code and will now load the recent project.
  };

  Type m_Type;
};

class XII_EDITORFRAMEWORK_DLL xiiQtEditorApp : public QObject
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtEditorApp);

public:
  struct StartupFlags
  {
    using StorageType = xiiUInt8;
    enum Enum
    {
      Headless   = XII_BIT(0), ///< The app does not do any rendering.
      SafeMode   = XII_BIT(1), ///< '-safe' : Prevent automatic loading of projects, scenes, etc. to minimize risk of crashing.
      NoRecent   = XII_BIT(2), ///< '-norecent' : Do not modify recent file lists. Used for modes such as tests, where the user does not do any interactions.
      UnitTest   = XII_BIT(3), ///< Specified when the process is running as a unit test
      Background = XII_BIT(4), ///< This process is an editor processor background process handling IPC tasks of the editor parent process.
      Default    = 0,
    };

    struct Bits
    {
      StorageType Headless : 1;
      StorageType SafeMode : 1;
      StorageType NoRecent : 1;
      StorageType UnitTest : 1;
      StorageType Background : 1;
    };
  };

public:
  xiiQtEditorApp();
  ~xiiQtEditorApp();

  static xiiEvent<const xiiEditorAppEvent&> m_Events;

  //
  // External Tools
  //

  /// \brief Searches for an external tool.
  ///
  /// Either uses one from the precompiled tools folder, or from the currently compiled binaries, depending where it finds one.
  /// If the editor preference is set to use precompiled tools, that folder is preferred, otherwise the other folder is preferred.
  xiiString FindToolApplication(const char* szToolName);

  /// \brief Executes an external tool as found by FindToolApplication().
  ///
  /// The applications output is parsed and forwarded to the given log interface. A custom log level is applied first.
  /// If the tool cannot be found or it takes longer to execute than the allowed timeout, the function returns failure.
  xiiStatus ExecuteTool(const char* szTool, const QStringList& arguments, xiiUInt32 uiSecondsTillTimeout, xiiLogInterface* pLogOutput = nullptr, xiiLogMsgType::Enum logLevel = xiiLogMsgType::WarningMsg, const char* szCWD = nullptr);

  /// \brief Creates the string with which to run Fileserve for the currently open project.
  xiiString BuildFileserveCommandLine() const;

  /// \brief Launches Fileserve with the settings for the current project.
  void RunFileserve();

  /// \brief Launches xiiInspector.
  void RunInspector();

  //
  //
  //

  /// \brief Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_StartupFlags.IsSet(StartupFlags::SafeMode); }

  /// \brief Returns true if the the app shouldn't display anything. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_StartupFlags.IsSet(StartupFlags::Headless); }

  /// \brief Returns true if the editor is started in run in test mode.
  bool IsInUnitTestMode() const { return m_StartupFlags.IsSet(StartupFlags::UnitTest); }

  /// \brief Returns true if the editor is started in run in background mode.
  bool IsBackgroundMode() const { return m_StartupFlags.IsSet(StartupFlags::Background); }

  const xiiPluginBundleSet& GetPluginBundles() const { return m_PluginBundles; }
  xiiPluginBundleSet&       GetPluginBundles() { return m_PluginBundles; }

  void                     AddRestartRequiredReason(const char* szReason);
  const xiiSet<xiiString>& GetRestartRequiredReasons() { return m_RestartRequiredReasons; }

  void                     AddReloadProjectRequiredReason(const char* szReason);
  const xiiSet<xiiString>& GetReloadProjectRequiredReason() { return m_ReloadProjectRequiredReasons; }

  void SaveSettings();

  /// \brief Writes a file containing all the currently open documents
  void SaveOpenDocumentsList();

  /// \brief Reads the list of last open documents in the current project.
  xiiRecentFilesList LoadOpenDocumentsList();

  void     InitQt(int iArgc, char** pArgv);
  void     StartupEditor();
  void     StartupEditor(xiiBitflags<StartupFlags> startupFlags, const char* szUserDataFolder = nullptr);
  void     ShutdownEditor();
  xiiInt32 RunEditor();
  void     DeInitQt();

  void LoadEditorPlugins();

  xiiRecentFilesList& GetRecentProjectsList() { return m_RecentProjects; }
  xiiRecentFilesList& GetRecentDocumentsList() { return m_RecentDocuments; }

  xiiEditorEngineProcessConnection* GetEngineViewProcess() { return m_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  void      CloseProject();
  xiiResult OpenProject(const char* szProject, bool bImmediate = false);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiOpenDashboard();
  void GuiOpenDocsAndCommunity();
  bool GuiCreateProject(bool bImmediate = false);
  bool GuiOpenProject(bool bImmediate = false);

  void         OpenDocumentQueued(xiiStringView sDocument, const xiiDocumentObject* pOpenContext = nullptr);
  xiiDocument* OpenDocument(xiiStringView sDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext = nullptr);
  xiiDocument* CreateDocument(xiiStringView sDocument, xiiBitflags<xiiDocumentFlags> flags, const xiiDocumentObject* pOpenContext = nullptr);

  xiiResult CreateOrOpenProject(bool bCreate, xiiStringView sFile);

  /// \brief If this project is remote, ie coming from another repository that is not checked-out by default, make sure it exists locally on disk.
  ///
  /// Adjusts inout_sFilePath from pointing to an xiiRemoteProject file to a xiiProject file, if necessary.
  /// If the project is already local, it always succeeds.
  /// If checking out fails or is user canceled, the function returns failure.
  xiiStatus MakeRemoteProjectLocal(xiiStringBuilder& inout_sFilePath);

  bool ExistsPluginSelectionStateDDL(const char* szProjectDir = ":project");
  void WritePluginSelectionStateDDL(const char* szProjectDir = ":project");
  void CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate);
  void LoadPluginBundleDlls(const char* szProjectFile);
  void DetectAvailablePluginBundles(xiiStringView sSearchDirectory);

  /// \brief Launches a new instance of the editor to open the given project.
  void LaunchEditor(const char* szProject, bool bCreate);

  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false);

  const xiiApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const xiiApplicationPluginConfig      GetRuntimePluginConfig(bool bIncludeEditorPlugins) const;

  void SetFileSystemConfig(const xiiApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(xiiStringBuilder& ref_sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(xiiString& ref_sPath) const;
  bool MakePathDataDirectoryRelative(xiiStringBuilder& ref_sPath) const;
  bool MakePathDataDirectoryRelative(xiiString& ref_sPath) const;

  bool MakePathDataDirectoryParentRelative(xiiStringBuilder& ref_sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(xiiStringBuilder& ref_sPath, bool bCheckExists) const;

  xiiStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/xiiEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(xiiDynamicArray<xiiString>& slots) const;

  /// \brief Instructs the engine to reload its resources
  void ReloadEngineResources();

  void RestartEngineProcessIfPluginsChanged(bool bForce);
  void SetStyleSheet();

Q_SIGNALS:
  void IdleEvent();

private:
  xiiString BuildDocumentTypeFileFilter(bool bForCreation);

  void GuiCreateOrOpenDocument(bool bCreate);
  bool GuiCreateOrOpenProject(bool bCreate);

private Q_SLOTS:
  void SlotTimedUpdate();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject, void* pOpenContext);
  void SlotQueuedGuiOpenDashboard();
  void SlotQueuedGuiOpenDocsAndCommunity();
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);
  void SlotSaveSettings();
  void SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced);

private:
  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(xiiDocumentManager::Request& r);
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& r);
  void DocumentEventHandler(const xiiDocumentEvent& e);
  void DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e);
  void ProjectRequestHandler(xiiToolsProjectRequest& r);
  void ProjectEventHandler(const xiiToolsProjectEvent& r);
  void EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e);
  void UiServicesEvents(const xiiQtUiServices::Event& e);

  void SetupNewProject();
  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void StoreEnginePluginModificationTimes();
  bool CheckForEnginePluginModifications();
  void SaveAllOpenDocuments();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void CreatePanels();

  void SetupAndShowSplashScreen();
  void CloseSplashScreen();

  xiiResult AddBundlesInOrder(xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig>& order, const xiiPluginBundleSet& bundles, const xiiString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const;

  bool m_bSavePreferencesAfterOpenProject;
  bool m_bLoadingProjectInProgress = false;
  bool m_bAnyProjectOpened         = false;
  bool m_bWroteCrashIndicatorFile  = false;

  xiiBitflags<StartupFlags>  m_StartupFlags;
  xiiDynamicArray<xiiString> m_DocumentsToOpen;

  xiiSet<xiiString> m_RestartRequiredReasons;
  xiiSet<xiiString> m_ReloadProjectRequiredReasons;

  xiiPluginBundleSet m_PluginBundles;

  void SaveRecentFiles();
  void LoadRecentFiles();

  xiiRecentFilesList m_RecentProjects;
  xiiRecentFilesList m_RecentDocuments;

  int                               m_iArgc          = 0;
  QApplication*                     m_pQtApplication = nullptr;
  xiiLongOpControllerManager        m_LongOpControllerManager;
  xiiEditorEngineProcessConnection* m_pEngineViewProcess = nullptr;
  QTimer*                           m_pTimer = nullptr;

  QSplashScreen* m_pSplashScreen = nullptr;

  xiiLogWriter::HTML m_LogHTML;

  xiiTime                        m_LastPluginModificationCheck;
  xiiApplicationFileSystemConfig m_FileSystemConfig;

  // *** Recent Paths ***
  xiiString m_sLastDocumentFolder;
  xiiString m_sLastProjectFolder;

  // *** Progress Bar ***
public:
  bool IsProgressBarProcessingEvents() const;

private:
  xiiProgress*      m_pProgressbar   = nullptr;
  xiiQtProgressbar* m_pQtProgressbar = nullptr;

  // *** Localization ***
  xiiTranslatorFromFiles* m_pTranslatorFromFiles = nullptr;

  // *** Dynamic Enum Strings ***
  xiiSet<xiiString> m_DynamicEnumStringsToClear;
  void              OnDemandDynamicStringEnumLoad(xiiStringView sEnumName, xiiDynamicStringEnum& e);

  xiiUniquePtr<xiiQtVersionChecker> m_pVersionChecker;
};

XII_DECLARE_FLAGS_OPERATORS(xiiQtEditorApp::StartupFlags);
