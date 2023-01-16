#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/GameState/GameStateBase.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class xiiWindowBase;
struct xiiWindowCreationDesc;
class xiiWorld;

/// Allows custom code to inject logic at specific points during
/// initialization or during shutdown. The events are listed in
/// the order in which they typically happen.
struct xiiGameApplicationStaticEvent
{
  enum class Type
  {
    AfterGameStateActivated,
    BeforeGameStateDeactivated
  };

  Type m_Type;
};

/// Allows custom code to inject logic at specific update points.
/// The events are listed in the order in which they typically happen.
struct xiiGameApplicationExecutionEvent
{
  enum class Type
  {
    BeginAppTick,
    BeforeWorldUpdates,
    AfterWorldUpdates,
    BeforeUpdatePlugins,
    AfterUpdatePlugins,
    BeforePresent,
    AfterPresent,
    EndAppTick,
  };

  Type m_Type;
};

// TODO: document this and update xiiGameApplication comments

class XII_CORE_DLL xiiGameApplicationBase : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  xiiGameApplicationBase(const char* szAppName);
  ~xiiGameApplicationBase();

  /// \name Basics
  ///@{

public:
  /// \brief Returns the xiiGameApplicationBase singleton
  static xiiGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

protected:
  static xiiGameApplicationBase* s_pGameApplicationBaseInstance;

  ///@}
  /// \name Capturing Data
  ///@{

public:
  /// \brief Does a profiling capture and writes it to disk at ':appdata'
  void TakeProfilingCapture();

  /// \brief Schedules a screenshot to be taken at the end of the frame.
  ///
  /// After taking a screenshot, StoreScreenshot() is executed, which may decide where to write the result to.
  void TakeScreenshot();

protected:
  /// \brief Called with the result from taking a screenshot. The default implementation writes the image to disk at ':appdata/Screenshots'
  virtual void StoreScreenshot(xiiImage&& image, const char* szContext = nullptr);

  void ExecuteTakeScreenshot(xiiWindowOutputTargetBase* pOutputTarget, const char* szContext = nullptr);

  bool m_bTakeScreenshot = false;

  /// expose TakeScreenshot() as a console function
  xiiConsoleFunction<void()> m_ConFunc_TakeScreenshot;

  ///@}
  /// \name Frame Captures
  ///@{

public:
  /// \brief Schedules a frame capture if the corresponding plugin is loaded.
  ///
  /// If continuous capture mode is enabled the currently running frame capture is persisted (and not discarded).
  /// Otherwise, the next frame will be captured and persisted.
  void CaptureFrame();

  /// \brief Controls if frame captures are taken continuously (without being persisted) or only on-demand.
  ///
  /// If continuous frame capture is enabled, calling CaptureFrame() will persist the result of the frame capture that is
  /// currently in progress. If continuous frame capture is disabled, CaptureFrame() will capture and persist the next frame.
  /// Note that continuous capture mode comes with a performance cost, but allows the user to decide on-the-fly if the current
  /// frame capture is to be persisted, e.g., when a unit test image comparison fails.
  void SetContinuousFrameCapture(bool enable);
  bool GetContinousFrameCapture() const;

  /// \brief Get the absolute base output path for frame captures.
  virtual xiiResult GetAbsFrameCaptureOutputPath(xiiStringBuilder& sOutputPath);

protected:
  void ExecuteFrameCapture(xiiWindowHandle targetWindowHandle, const char* szContext = nullptr);

  bool m_bContinuousFrameCapture = false;
  bool m_bCaptureFrame           = false;

  /// expose CaptureFrame() as a console function
  xiiConsoleFunction<void()> m_ConFunc_CaptureFrame;

  ///@}
  /// \name GameState
  ///@{
public:
  /// \brief Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ///
  /// Broadcasts local event: xiiGameApplicationStaticEvent::AfterGameStateActivated
  /// Broadcasts global event: AfterGameStateActivation(xiiGameStateBase*)
  xiiResult ActivateGameState(xiiWorld* pWorld = nullptr, const xiiTransform* pStartPosition = nullptr);

  /// \brief Deactivates and destroys the active game state.
  ///
  /// Broadcasts local event: xiiGameApplicationStaticEvent::BeforeGameStateDeactivated
  /// Broadcasts global event: BeforeGameStateDeactivation(xiiGameStateBase*)
  void DeactivateGameState();

  /// \brief Returns the currently active game state. Could be nullptr.
  xiiGameStateBase* GetActiveGameState() const { return m_pGameState.Borrow(); }

  /// \brief Returns the currently active game state IF it was created for the given world.
  ///
  /// This is mostly for editor use cases, where some documents want to handle the game state, but only
  /// it it was set up for a particular document.
  xiiGameStateBase* GetActiveGameStateLinkedToWorld(const xiiWorld* pWorld) const;

protected:
  /// \brief Creates a game state for the application to use.
  ///
  /// \a pWorld is typically nullptr in a stand-alone app, but may be existing already when called from the editor.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState(xiiWorld* pWorld);

  /// \brief Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  xiiUniquePtr<xiiGameStateBase> m_pGameState;
  xiiWorld*                      m_pWorldLinkedWithGameState = nullptr;

  ///@}
  /// \name Platform Profile
  ///@{
public:
  /// \brief Returns the xiiPlatformProfile that has been loaded for this application
  const xiiPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }


protected:
  xiiPlatformProfile m_PlatformProfile;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;

  /// \brief Returns the target of the 'project' special data directory.
  ///
  /// The return value of this function will be passed into xiiFileSystem::SetSpecialDirectory.
  /// Afterwards, any path starting with the special directory marker (">project/") will point
  /// into this directory.
  virtual xiiString FindProjectDirectory() const = 0;

  /// \brief Returns the target of the 'base' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'base' data directory. Target defaults to ">sdk/Data/Base".
  virtual xiiString GetBaseDataDirectoryPath() const;

  /// \brief Returns the target of the 'project' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'project' data directory. Target defaults to ">project/".
  virtual xiiString GetProjectDataDirectoryPath() const;

  /// \brief Executes all 'BaseInit_' functions. Typically done very early, before core system startup
  virtual void ExecuteBaseInitFunctions();
  virtual void BaseInit_ConfigureLogging();

  xiiEventSubscriptionID m_LogToConsoleID = 0;
  xiiEventSubscriptionID m_LogToVsID      = 0;

  /// \brief Executes all 'Init_' functions. Typically done after core system startup
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

  xiiCopyOnBroadcastEvent<const xiiGameApplicationExecutionEvent&> m_ExecutionEvents;

  xiiTime GetFrameTime() const { return m_FrameTime; }

protected:
  virtual bool IsGameUpdateEnabled() const { return true; }

  virtual void Run_InputUpdate();
  virtual bool Run_ProcessApplicationInput();
  virtual void Run_WorldUpdateAndRender() = 0;
  virtual void Run_BeforeWorldUpdate();
  virtual void Run_AfterWorldUpdate();
  virtual void Run_UpdatePlugins();
  virtual void Run_Present();
  virtual void Run_FinishFrame();

  void UpdateFrameTime();

  xiiTime m_FrameTime;
  ///@}
};
