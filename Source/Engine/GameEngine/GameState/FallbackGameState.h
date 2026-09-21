/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utilities/SceneLoadUtil.h>

class xiiCameraComponent;

/// xiiFallbackGameState is an xiiGameState that can handle existing worlds when no other game state is available.
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

  virtual void ProcessInput() override;

  virtual void OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;

  /// Reports true for xiiFallbackGameState only, not for derived types.
  virtual bool IsFallbackGameState() const override;

protected:
  /// Called by SwitchToLoadingScreen() to setup a new world that acts as the loading screen while waiting for another scene to finish loading.
  virtual void      ConfigureInputActions() override;
  virtual xiiResult SpawnPlayer(xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;

  virtual const xiiCameraComponent* FindActiveCameraComponent();

  xiiInt32 m_iActiveCameraComponentIndex = -3;

  //////////////////////////////////////////////////////////////////////////

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  State m_State     = State::Ok;
  bool  m_bShowMenu = false;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool                       m_bCheckedForScenes = false;
  xiiDynamicArray<xiiString> m_AvailableScenes;
  xiiUInt32                  m_uiSelectedScene = 0;
  xiiString                  m_sTitleOfScene;

  virtual void OnBackgroundSceneLoadingFinished(xiiUniquePtr<xiiWorld>&& pWorld) override;
  virtual void OnBackgroundSceneLoadingFailed(xiiStringView sReason) override;

  virtual void ConfigureMainCamera() override;
};
