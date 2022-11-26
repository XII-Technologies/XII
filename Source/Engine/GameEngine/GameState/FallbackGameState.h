#pragma once

#include <Core/Graphics/Camera.h>
#include <GameEngine/GameState/GameState.h>

class xiiCameraComponent;

/// \brief xiiFallbackGameState is an xiiGameState that can handle existing worlds when no other game state is available.
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
  virtual void AfterWorldUpdate() override;

  /// \brief Returns Priority::None if pWorld == nullptr, Priority::Fallback otherwise.
  virtual xiiGameStatePriority DeterminePriority(xiiWorld* pWorld) const override;

protected:
  virtual void      ConfigureInputActions() override;
  virtual xiiResult SpawnPlayer(const xiiTransform* pStartPosition) override;

  virtual const xiiCameraComponent* FindActiveCameraComponent();

  xiiInt32 m_iActiveCameraComponentIndex;
};
