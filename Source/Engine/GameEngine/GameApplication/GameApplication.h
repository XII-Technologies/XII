/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/GameState/GameState.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Configuration/GameApplicationSettings.h>

class xiiQuakeConsole;
struct xiiGALDeviceCreationDescription;

// TODO: update comments below

/// The base class for all typical game applications made with xiiEngine
///
/// While xiiApplication is an abstraction for the operating system entry point,
/// xiiGameApplication extends this to implement startup and tear down functionality
/// of a typical game that uses the standard functionality of xiiEngine.
///
/// xiiGameApplication implements a lot of functionality needed by most games,
/// such as setting up data directories, loading plugins, configuring the input system, etc.
///
/// For every such step a virtual function is called, allowing to override steps in custom applications.
///
/// The default implementation tries to do as much of this in a data-driven way. E.g. plugin and data
/// directory configurations are read from DDL files. These can be configured by hand or using xiiEditor.
///
/// You are NOT supposed to implement game functionality by deriving from xiiGameApplication.
/// Instead see xiiGameState.
///
/// xiiGameApplication will create exactly one xiiGameState by looping over all available xiiGameState types
/// (through reflection) and picking the one whose DeterminePriority function returns the highest priority.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class XII_GAMEENGINE_DLL xiiGameApplication : public xiiGameApplicationBase
{
public:
  static xiiCVarBool cvar_AppVSync;

  const xiiGameApplicationSettings& GetSettings() const { return m_Settings; }
  /// Commits validated configuration and notifies output targets once.
  xiiResult ApplySettings(const xiiGameApplicationSettings& settings);

public:
  using SUPER = xiiGameApplicationBase;

  /// sProjectPath may be empty, if FindProjectDirectory() is overridden.
  xiiGameApplication(xiiStringView sAppName, xiiStringView sProjectPath = {});
  ~xiiGameApplication();

  /// Returns the xiiGameApplication singleton
  static xiiGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// Transfers an optional already-responsive startup window to the game state. Called once by CreateMainWindow().
  virtual xiiUniquePtr<xiiWindow> TakeStartupWindow() { return {}; }

  xiiApplication::Execution Run() override;

  /// Returns the active renderer of the current app. Either the default or overridden via -render command line flag.
  static xiiStringView GetActiveRenderer();

  /// When the graphics device is created, by default the game application will pick a platform specific implementation. This
  /// function allows to override that by setting a custom function that creates a graphics device.
  static void SetOverrideDefaultDeviceCreator(xiiDelegate<xiiSharedPtr<xiiGALDevice>(const xiiGALDeviceCreationDescription&)> creator);

  /// Implementation of xiiGameApplicationBase::FindProjectDirectory to define the 'project' special data directory.
  ///
  /// The default implementation will try to resolve m_sAppProjectPath to an absolute path. m_sAppProjectPath can be absolute itself,
  /// relative to ">sdk/" or relative to xiiOSFile::GetApplicationDirectory().
  /// m_sAppProjectPath must be set either via the xiiGameApplication constructor or manually set before project.
  ///
  /// Alternatively, xiiGameApplication::FindProjectDirectory() must be overwritten.
  virtual xiiString FindProjectDirectory() const override;

  /// Used at runtime (by the editor) to reload input maps. Forwards to Init_ConfigureInput()
  void ReinitializeInputConfig();

  /// Returns the project path that was given to the constructor (or modified by an overridden implementation).
  xiiStringView GetAppProjectPath() const { return m_sAppProjectPath; }

protected:
  void      AfterCoreSystemsStartup() override;
  void      BeforeHighLevelSystemsShutdown() override;
  void      BeforeCoreSystemsShutdown() override;
  void      Run_InputUpdate() override;
  xiiUInt32 GetFrameLimit() const override;
  void      ApplyViewSettings();
  xiiUInt64 m_uiConfiguredWorldRevision = 0;
  void      Init_FileSystem_ConfigureDataDirs() override;
  void      Init_ConfigureTelemetry() override;

  /// Custom staged startup calls this after core startup, before its first initialization stage.
  void         InitializeManagedSession();
  void         PumpSession();
  virtual void ReportStatus(xiiStringView sStage, xiiStringView sDetail, float fProgress = -1.0f);

  xiiString                                  m_sInstance;
  xiiString                                  m_sSession;
  xiiString                                  m_sStage = "initializing";
  xiiString                                  m_sDetail;
  xiiString                                  m_sManagedWritableDirectory;
  xiiUniquePtr<xiiIpcChannel>                m_pSessionChannel;
  xiiUniquePtr<xiiIpcProcessMessageProtocol> m_pSessionProtocol;
  xiiTime                                    m_LastStatus;
  xiiTime                                    m_LastConnectAttempt;
  bool                                       m_bSessionConnected = false;
  xiiOSFile                                  m_ManagedPreferencesLock;

  virtual void Init_ConfigureInput() override;
  virtual void Init_ConfigureCVars() override;
  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual bool Run_ProcessApplicationInput() override;
  virtual void Run_WorldUpdateAndRender() override;
  virtual void Run_PresentImage() override;
  virtual void Run_FinishFrame() override;

  /// Stores what is given to the constructor
  xiiString m_sAppProjectPath;
  bool      m_bIgnoreErrors = false;

protected:
  static xiiGameApplication* s_pGameApplicationInstance;

  void RenderConsole();

  void OnVSyncChanged(const xiiCVarEvent& e);
  void OnSettingsChanged(xiiSettings* pSettings);

protected:
  static xiiDelegate<xiiSharedPtr<xiiGALDevice>(const xiiGALDeviceCreationDescription&)> s_DefaultDeviceCreator;

  bool                          m_bShowConsole = false;
  xiiUniquePtr<xiiQuakeConsole> m_pConsole;

  xiiEventSubscriptionID               m_CVarChangeSubscriptionID;
  xiiEventSubscriptionID               m_FrameLimitSubscriptionID = 0;
  xiiGameApplicationSettings           m_Settings;
  xiiEvent<xiiSettings*>::Unsubscriber m_SettingsSubscription;
  bool                                 m_bSettingsInitialized = false;
  bool                                 m_bSettingsWritable    = true;
};
