/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/GameState/GameStateBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Utilities/SceneLoadUtil.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

class xiiView;
class xiiWindow;
class xiiWindowOutputTargetBase;
class xiiWindowOutputTargetGAL;

struct xiiWindowEvent;

/// xiiGameState implements the xiiGameStateBase interface and adds several convenience features.
///
/// For an explanation what game states are, see the online documentation:
/// https://xiiengine.net/pages/docs/runtime/application/game-state.html
///
/// The xiiGameState adds some default functionality:
/// * Creation of a main window and render pipeline.
/// * A main view handle.
/// * A main camera object.
/// * A main world that is currently active.
/// * Background loading of scenes.
/// * A separate world used as a loading screen.
/// * automatic player prefab spawning if a xiiPlayerStartPointComponent is part of the scene.
/// * automatically applies the state of the "Main View" xiiCameraComponent in the scene.
/// * Many additional hooks to customize only specific parts, such as the window creation.
///
/// Typically you would derive from xiiGameState and then override functions like `ProcessInput()` and `ConfigureMainCamera()`. Take a look at `xiiFallbackGameState` for inspiration.
class XII_GAMEENGINE_DLL xiiGameState : public xiiGameStateBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameState, xiiGameStateBase)

protected:
  /// This class cannot be instantiated directly.
  xiiGameState();

public:
  virtual ~xiiGameState();

  /// Returns the active xiiGameState. Only one xiiGameState is allowed to exist.
  static xiiGameState* GetActiveGameState();

  /// Returns the xiiWorld that is currently the active one.
  xiiWorld* GetMainWorld() { return m_pMainWorld; }

  /// Gives access to the game state's main camera object.
  xiiCamera* GetMainCamera() { return &m_MainCamera; }

  /// Returns the xiiView that is currently the one used for rendering the main output.
  xiiView* GetMainView();

  /// Whether a scene is currently being loaded.
  bool IsLoadingSceneInBackground(float* out_pProgress = nullptr) const;

  /// Whether the game state currently displays a loading screen. This usually implies that a scene is being loaded as well.
  bool IsInLoadingScreen() const;

  /// Called upon game startup.
  ///
  /// Calls CreateWindows() to create the game's main window and setup input devices.
  /// Calls ConfigureInputActions() to setup input actions.
  /// Finally switches to pWorld (if available) or starts loading the scene that GetStartupOptions() returns.
  ///
  /// Override any of the above functions to customize them.
  virtual void OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;

  /// Cleans up the main window before the game is shut down.
  virtual void OnDeactivation() override;

  /// Simply stores that the game should stop.
  ///
  /// Override this to add more elaborate logic, if necessary.
  virtual void RequestQuit(xiiStringView sRequestedBy) override;

  /// Whether WasQuitRequested() was called before.
  virtual bool WasQuitRequested() const override;

  /// The xiiGameState doesn't implement any input logic, but it forwards to UpdateBackgroundSceneLoading().
  virtual void ProcessInput() override;

  /// Immediately switches to a loading screen and starts loading a level.
  ///
  /// When the level is finished loading, `OnBackgroundSceneLoadingFinished()` is called, which switches to it immediately, unless overridden.
  /// If the scene was already fully preloaded, the switch happens immediately, without showing a loading screen.
  void LoadScene(xiiStringView sSceneFile, xiiStringView sPreloadCollection, xiiStringView sStartPosition, const xiiTransform& startPositionOffset);

  /// Convenience function to switch to a loading screen.
  ///
  /// Nothing actually gets loaded. Without further logic, the app will stay in the loading screen indefinitely.
  /// sTargetSceneFile is only passed in, so that the loading screen can be customized accordingly,
  /// for example it may show a screenshot of the target scene.
  void SwitchToLoadingScreen(xiiStringView sTargetSceneFile);

  /// Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  ///
  /// Calls OnChangedMainWorld() afterwards, so that you can follow up on a scene change as needed.
  /// Calls ConfigureMainCamera() as well.
  void ChangeMainWorld(xiiWorld* pNewMainWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset);

  /// Starts loading a scene in the background.
  ///
  /// If available, a collection can be provided. Resources referenced in the collection will be fully preloaded first and then
  /// the scene is loaded. This is the only way to get a proper estimation of loading progress and is necessary to get a smooth
  /// start, otherwise the engine will have to load resources on-demand, many of which will be needed during the first frame.
  ///
  /// Once finished, one of these hooks is executed:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  ///   `OnBackgroundSceneLoadingCanceled()`
  void StartBackgroundSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollection);

  /// If a scene is currently being loaded in the background, cancel the loading.
  ///
  /// Calls `OnBackgroundSceneLoadingCanceled()` if a scene was loading.
  void CancelBackgroundSceneLoading();

protected:
  /// Creates an actor with a default window (xiiGameStateWindow) adds it to the application
  ///
  /// The base implementation calls CreateMainWindow(), CreateMainOutputTarget() and SetupMainView() to configure the main window.
  virtual void CreateWindows();

  /// Adds custom input actions, if necessary.
  /// Unless overridden OnActivation() will call this.
  virtual void ConfigureInputActions();

  /// Overrideable function that may create a player object.
  ///
  /// By default called by OnChangedMainWorld() when switching to a non-loading screen world.
  /// The default implementation will search the world for xiiPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the xiiPlayerStartComponent will be used.
  ///
  /// sStartPosition allows to spawn the player at another named location, but there is no default implementation for such logic.
  ///
  /// Returns XII_SUCCESS if a prefab was spawned, XII_FAILURE if nothing was done.
  virtual xiiResult SpawnPlayer(xiiStringView sStartPosition, const xiiTransform& startPositionOffset);

  /// Creates a default main view.
  xiiView* CreateMainView();

  /// Executed when ChangeMainWorld() is used to switch to a new world.
  ///
  /// Override this to be informed about scene changes.
  /// This happens right at startup (both for given worlds and custom created ones)
  /// and when the game needs to switch to a new level.
  virtual void OnChangedMainWorld(xiiWorld* pPreviousWorld, xiiWorld* pNewWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset);

  /// Searches for a "Main View" xiiCameraComponent in the world and uses that for the camera position, if available.
  ///
  /// Override this for custom camera logic.
  virtual void ConfigureMainCamera() override;

  /// Override this to modify the default window creation behavior. Called by CreateWindows().
  virtual xiiUniquePtr<xiiWindow> CreateMainWindow();

  /// Override this to modify the default output target creation behavior. Called by CreateWindows().
  virtual xiiUniquePtr<xiiWindowOutputTargetGAL> CreateMainOutputTarget(xiiWindow* pMainWindow);

  /// Creates a default render view. Unless overridden, OnActivation() will do this for the main window.
  virtual void SetupMainView(xiiGALSwapChain* pSwapChain, xiiSizeU32 viewportSize);

  /// Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Called by CreateWindows() with the result of CreateMainWindow().
  virtual void ConfigureMainWindowInputDevices(xiiWindow* pWindow);

  /// Returns the path to the scene file and the corresponding preload collection to load at startup.
  ///
  /// By default this is taken from the command line '-scene' option.
  /// Override this function to define a custom startup scene (e.g. for the main menu) or load a saved state.
  virtual void GetStartupOptions(xiiString& out_sScene, xiiString& out_sPreloadCollection);

  /// Called by SwitchToLoadingScreen() to set up a new loading screen world.
  ///
  /// A loading screen uses a separate xiiWorld. It can be fully set up in code or loaded from disk,
  /// but it should be very light-weight, so that it is quick to set up.
  virtual xiiUniquePtr<xiiWorld> CreateLoadingScreenWorld(xiiStringView sTargetSceneFile);

  /// If a scene is being loaded in the background, this advanced the loading.
  ///
  /// Upon success or failure, executes either of these:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  void UpdateBackgroundSceneLoading();

  /// Called by `UpdateBackgroundSceneLoading()` when a scene is finished loading.
  ///
  /// May switch to the scene immediately or wait, for example for a user to confirm.
  virtual void OnBackgroundSceneLoadingFinished(xiiUniquePtr<xiiWorld>&& pWorld);

  /// Called by `UpdateBackgroundSceneLoading()` when a scene failed to load.
  virtual void OnBackgroundSceneLoadingFailed(xiiStringView sReason);

  /// Called by `CancelBackgroundSceneLoading()` when scene loading gets canceled.
  virtual void OnBackgroundSceneLoadingCanceled();

  /// Forwards window events from the platform window. Override this to react to window events, such as resizing.
  virtual void OnWindowEvent(const xiiWindowEvent& e);

protected:
  static xiiGameState* s_pActiveGameState;

  xiiWorld*     m_pMainWorld = nullptr;
  xiiCamera     m_MainCamera;
  xiiViewHandle m_hMainView;

  bool m_bStateWantsToQuit = false;

  bool                              m_bTransitionWhenReady = false;
  xiiUniquePtr<xiiSceneLoadUtility> m_pBackgroundSceneLoad;
  xiiUniquePtr<xiiWorld>            m_pLoadingScreenWorld;
  xiiUniquePtr<xiiWorld>            m_pLoadedWorld;

  xiiString    m_sTargetSceneSpawnPoint;
  xiiTransform m_TargetSceneSpawnOffset = xiiTransform::MakeIdentity();
};
