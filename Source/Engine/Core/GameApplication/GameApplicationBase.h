/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Core/Console/ConsoleFunction.h>
#include <Core/GameState/GameStateBase.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class xiiWorld;
class xiiWindowBase;

struct xiiWindowCreationDescription;

/// Allows custom code to inject logic at specific points during initialization or during shutdown.
///
/// The events are listed in the order in which they typically happen.
struct xiiGameApplicationStaticEvent
{
  enum class Type : xiiUInt8
  {
    AfterGameStateActivated = 0U, ///< This is the first event that is triggered during the lifetime of the application. It is triggered after the game state has been activated, but before any worlds have been created or any scenes have been loaded.
    BeforeGameStateDeactivated    ///< This is the last event that is triggered during the lifetime of the application. It is triggered before the game state is deactivated, but after all worlds have been destroyed and all scenes have been unloaded.
  };

  Type m_Type;
};

/// Allows custom code to inject logic at specific update points.
///
/// The events are listed in the order in which they typically happen.
struct xiiGameApplicationExecutionEvent
{
  enum class Type : xiiUInt8
  {
    BeginAppTick = 0U,   ///< This is the first event that is triggered during the update phase of the application. It is triggered before any input is processed or any world is updated.
    BeforeWorldUpdates,  ///< This event is triggered after input has been processed, but before any world is updated. It is triggered once per frame, even if the application is paused.
    AfterWorldUpdates,   ///< This event is triggered after all worlds have been updated, but before any views are extracted. It is triggered once per frame, even if the application is paused.
    BeforeUpdatePlugins, ///< This event is triggered after all worlds have been updated and all views have been extracted, but before any plugins are updated. It is triggered once per frame, even if the application is paused.
    AfterUpdatePlugins,  ///< This event is triggered after all plugins have been updated, but before any rendering has started. It is triggered once per frame, even if the application is paused.
    BeforePresent,       ///< This event is triggered after all rendering has been completed, but before the back buffer is presented. It is triggered once per frame, even if the application is paused.
    AfterPresent,        ///< This is the last event that is triggered during the update phase of the application. It is triggered after the back buffer has been presented, but before any new frame has started. It is triggered once per frame, even if the application is paused.
    EndAppTick,          ///< This event is triggered at the very end of the update phase of the application, after all other events have been triggered. It is triggered once per frame, even if the application is paused.
  };

  Type m_Type;
};

/// The xiiGameApplicationBase class is the base class for all game applications. It provides common functionality for managing the game state, taking screenshots, and capturing frames.
class XII_CORE_DLL xiiGameApplicationBase : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiGameApplicationBase(xiiStringView sAppName);
  ~xiiGameApplicationBase();

  /// \name Basics
  ///@{

public:
  /// Returns the xiiGameApplicationBase singleton
  static xiiGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

protected:
  static xiiGameApplicationBase* s_pGameApplicationBaseInstance;

  ///@}
  /// \name Capturing Data
  ///@{

public:
  /// Does a profiling capture and writes it to disk at ':appdata'
  void TakeProfilingCapture();

  /// Schedules a screenshot to be taken at the end of the frame.
  ///
  /// After taking a screenshot, StoreScreenshot() is executed, which may decide where to write the result to.
  void TakeScreenshot();

protected:
  /// Called with the result from taking a screenshot. The default implementation writes the image to disk at ':appdata/Screenshots'
  virtual void StoreScreenshot(xiiImage&& image, xiiStringView sContext = {});

  void ExecuteTakeScreenshot(xiiWindowOutputTargetBase* pOutputTarget, xiiStringView sContext = {});

  bool m_bTakeScreenshot = false;

  xiiConsoleFunction<void()> m_ConFunc_TakeScreenshot; ///< Expose TakeScreenshot() as a console function, so that it can be triggered by automated tests, e.g., when an image comparison fails, and game code.

  ///@}
  /// \name Frame Captures
  ///@{

public:
  /// Schedules a frame capture if the corresponding plugin is loaded.
  ///
  /// If continuous capture mode is enabled the currently running frame capture is persisted (and not discarded).
  /// Otherwise, the next frame will be captured and persisted.
  void CaptureFrame();

  /// Controls if frame captures are taken continuously (without being persisted) or only on-demand.
  ///
  /// If continuous frame capture is enabled, calling CaptureFrame() will persist the result of the frame capture that is
  /// currently in progress. If continuous frame capture is disabled, CaptureFrame() will capture and persist the next frame.
  /// Note that continuous capture mode comes with a performance cost, but allows the user to decide on-the-fly if the current
  /// frame capture is to be persisted, e.g., when a unit test image comparison fails.
  void SetContinuousFrameCapture(bool bEnable);
  bool GetContinousFrameCapture() const;

  /// Get the absolute base output path for frame captures.
  virtual xiiResult GetAbsFrameCaptureOutputPath(xiiStringBuilder& ref_sOutputPath);

protected:
  void ExecuteFrameCapture(xiiWindowHandle targetWindowHandle, xiiStringView sContext = {});

  bool m_bContinuousFrameCapture = false;
  bool m_bCaptureFrame           = false;

  /// expose CaptureFrame() as a console function
  xiiConsoleFunction<void()> m_ConFunc_CaptureFrame;

  ///@}
  /// \name GameState
  ///@{
public:
  /// Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ///
  /// Broadcasts local event: xiiGameApplicationStaticEvent::AfterGameStateActivated
  /// Broadcasts global event: AfterGameStateActivation(xiiGameStateBase*)
  void ActivateGameState(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset);

  /// Deactivates and destroys the active game state.
  ///
  /// Broadcasts local event: xiiGameApplicationStaticEvent::BeforeGameStateDeactivated
  /// Broadcasts global event: BeforeGameStateDeactivation(xiiGameStateBase*)
  void DeactivateGameState();

  /// Returns the currently active game state. Could be nullptr.
  xiiGameStateBase* GetActiveGameState() const { return m_pGameState.Borrow(); }

protected:
  /// Creates a game state for the application to use.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState();

  /// Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  xiiUniquePtr<xiiGameStateBase> m_pGameState;

  ///@}
  /// \name Platform Profile
  ///@{
public:
  /// Returns the xiiPlatformProfile that has been loaded for this application
  const xiiPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }


protected:
  xiiPlatformProfile m_PlatformProfile;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;

  /// Returns the target of the 'project' special data directory.
  ///
  /// The return value of this function will be passed into xiiFileSystem::SetSpecialDirectory.
  /// Afterwards, any path starting with the special directory marker (">project/") will point
  /// into this directory.
  virtual xiiString FindProjectDirectory() const = 0;

  /// Returns the target of the 'base' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'base' data directory. Target defaults to ">sdk/Data/Base".
  virtual xiiString GetBaseDataDirectoryPath() const;

  /// Returns the target of the 'project' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'project' data directory. Target defaults to ">project/".
  virtual xiiString GetProjectDataDirectoryPath() const;

  /// Executes all 'BaseInit_' functions. Typically done very early, before core system startup
  virtual void ExecuteBaseInitFunctions();
  virtual void BaseInit_ConfigureLogging();

  xiiEventSubscriptionID m_LogToConsoleID = 0;
  xiiEventSubscriptionID m_LogToVsID      = 0;

  /// Executes all 'Init_' functions. Typically done after core system startup
  virtual void ExecuteInitFunctions();
  virtual void Init_PlatformProfile_SetPreferred();
  virtual void Init_ConfigureTelemetry();
  virtual void Init_FileSystem_SetSpecialDirs();
  virtual void Init_LoadRequiredPlugins();
  virtual void Init_ConfigureAssetManagement();
  virtual void Init_FileSystem_ConfigureDataDirs();
  virtual void Init_LoadWorldModuleConfig();
  virtual void Init_LoadProjectPlugins();
  virtual void Init_PlatformProfile_LoadForRuntime();
  virtual void Init_ConfigureInput();
  virtual void Init_ConfigureTags();
  virtual void Init_ConfigureCVars();
  virtual void Init_SetupGraphicsDevice() = 0;
  virtual void Init_SetupDefaultResources();

  xiiEvent<const xiiGameApplicationStaticEvent&> m_StaticEvents;

  ///@}
  /// \name Application Shutdown
  ///@{
protected:
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Deinit_ShutdownGraphicsDevice() = 0;
  virtual void Deinit_UnloadPlugins();
  virtual void Deinit_ShutdownLogging();

  ///@}
  /// \name Application Execution
  ///@{

public:
  virtual xiiApplication::Execution Run() override;

  void RunOneFrame();

  xiiCopyOnBroadcastEvent<const xiiGameApplicationExecutionEvent&> m_ExecutionEvents;

  xiiTime GetFrameTime() const { return m_FrameTime; }

protected:
  virtual void Run_InputUpdate();
  virtual bool Run_ProcessApplicationInput();
  virtual void Run_WorldUpdateAndRender() = 0;
  virtual void Run_BeforeWorldUpdate();
  virtual void Run_AfterWorldUpdate();
  virtual void Run_UpdatePlugins();
  /// This function can be used to present the final image to a window. It is run at the end of the rendering phase. It can also be used to inspect the swap-chain e.g. for screenshot purposes before presenting.
  virtual void Run_PresentImage();
  virtual void Run_FinishFrame();

  void UpdateFrameTime();

  xiiTime m_FrameTime;
  ///@}
};
