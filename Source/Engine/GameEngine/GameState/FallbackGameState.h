#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

class xiiCameraComponent;

using xiiCollectionResourceHandle = xiiTypedResourceHandle<class xiiCollectionResource>;

/// \brief xiiFallbackGameState is a xiiGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 'Fallback' in DeterminePriority() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for xiiEditor and xiiPlayer which make sure that a world already exists.
class XII_GAMEENGINE_DLL xiiFallbackGameState : public xiiGameState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFallbackGameState, xiiGameState)

public:
  xiiFallbackGameState();

  /// \brief If disabled, pressing the Windows key won't show an onscreen menu to switch to a different scene.
  void EnableSceneSelectionMenu(bool bEnable);

  /// \brief If disabled, moving around a scene with a free camera won't be possible.
  ///
  /// Also switching between scene cameras with Page Up/Down will be disabled.
  void EnableFreeCameras(bool bEnable);

  /// \brief If disabled, the game state will not automatically switch to a scene that is being loaded in the background,
  /// once it is finished loading. Instead overriding code has to call SwitchToLoadedScene() itself.
  void EnableAutoSwitchToLoadedScene(bool bEnable);

  virtual void ProcessInput() override;
  virtual void AfterWorldUpdate() override;

  /// \brief Returns xiiGameStatePriority::Fallback.
  virtual xiiGameStatePriority DeterminePriority(xiiWorld* pWorld) const override;

  virtual void OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition) override;
  virtual void OnDeactivation() override;

  /// \brief Returns the path to the scene file to load at startup. By default this is taken from the command line '-scene' option.
  virtual xiiString GetStartupSceneFile();

  /// \brief Creates a new world that's used as a temporary loading screen while waiting for loading of another world to finish.
  ///
  /// Usually this world would be set up in code and would be very quick to create. By default an entirely empty world is created.
  void SwitchToLoadingScreen();

  /// \brief Starts loading a scene in the background.
  ///
  /// If available, a collection can be provided. Resources referenced in the collection will be fully preloaded first and then
  /// the scene is loaded. This is the only way to get a proper estimation of loading progress and is necessary to get a smooth
  /// start, otherwise the engine will have to load resources on-demand, many of which will be needed during the first frame.
  xiiResult StartSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollection);

  /// \brief If a scene is currently being loaded in the background, cancel the loading.
  void CancelSceneLoading();

  /// \brief Whether a scene is currently being loaded.
  bool IsLoadingScene() const;

  /// \brief Whether the game state currently displays a loading screen. This usually implies that a scene is being loaded as well.
  bool IsInLoadingScreen() const;

  /// \brief Once scene loading has finished successfully, this can be called to switch to that scene.
  void SwitchToLoadedScene();

  /// \brief Returns the name of the xiiWorld that is currently active.
  xiiStringView GetActiveSceneName() const { return m_sTitleOfActiveScene; }

  /// \brief Returns the name of the xiiWorld that is currently being loaded.
  xiiStringView GetLoadingSceneName() const { return m_sTitleOfLoadingScene; }

protected:
  /// \brief Called by SwitchToLoadingScreen() to setup a new world that acts as the loading screen while waiting for another scene to finish loading.
  virtual xiiUniquePtr<xiiWorld> CreateLoadingScreenWorld();
  virtual void                   ConfigureInputActions() override;
  virtual xiiResult              SpawnPlayer(const xiiTransform* pStartPosition) override;

  virtual const xiiCameraComponent* FindActiveCameraComponent();

  xiiInt32 m_iActiveCameraComponentIndex;

  xiiUniquePtr<xiiWorld> m_pActiveWorld;

  xiiUniquePtr<xiiSceneLoadUtility> m_pSceneToLoad;

  //////////////////////////////////////////////////////////////////////////

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  State m_State                     = State::Ok;
  bool  m_bShowMenu                 = false;
  bool  m_bEnableSceneSelectionMenu = true;
  bool  m_bEnableFreeCameras        = true;
  bool  m_bIsInLoadingScreen        = false;
  bool  m_bAutoSwitchToLoadedScene  = true;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool                       m_bCheckedForScenes = false;
  xiiDynamicArray<xiiString> m_AvailableScenes;
  xiiUInt32                  m_uiSelectedScene = 0;
  xiiString                  m_sTitleOfLoadingScene;
  xiiString                  m_sTitleOfActiveScene;
};
