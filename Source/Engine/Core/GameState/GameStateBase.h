/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class xiiWorld;

/// \brief xiiGameStateBase is the base class for all game states. Game states are used to implement custom high level game logic.
///
/// See the online documentation for details: https://xiiengine.net/pages/docs/runtime/application/game-state.html
///
/// Note that you would typically derive custom game states from xiiGameState, not xiiGameStateBase, since the
/// former provides much more functionality out of the box.
class XII_CORE_DLL xiiGameStateBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameStateBase, xiiReflectedClass)

public:
  xiiGameStateBase()          = default;
  virtual ~xiiGameStateBase() = default;

  /// \brief A game state gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  ///
  /// \param sStartPosition
  /// An optional string to identify where the player should spawn.
  /// This may, for instance, be the unique name of an object. It is up to the game state how the string is used, if at all.
  ///
  /// \param pStartPosition
  /// An optional transform for the 'player object' to start at.
  /// Usually nullptr, but may be set by the editor to relocate or create the player object at the given destination.
  virtual void OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset) = 0;

  /// \brief Called when the game state is being shut down.
  ///
  /// Override this to clean up or save data to disk.
  virtual void OnDeactivation() = 0;

  /// \brief Called once per game update, early in the frame. Should handle input updates here.
  virtual void ProcessInput() {}

  /// \brief Called once each frame before the worlds are updated.
  virtual void BeforeWorldUpdate() {}

  /// \brief Called once each frame after the worlds have been updated.
  virtual void AfterWorldUpdate() {}

  /// \brief Called once each frame to configure the main camera position and rotation.
  ///
  /// Note that xiiCameraComponent may already apply set general options like field-of-view,
  /// so don't override these values, if you want to use that component.
  /// The default xiiGameState implementation searches for an xiiCameraComponent in the world that is set to "Main View"
  /// and uses it's transform for the main camera.
  virtual void ConfigureMainCamera() {}

  /// \brief Has to call xiiRenderLoop::AddMainView for all views that need to be rendered.
  ///
  /// This will be called every frame by the editor, to ensure that only the relevant views are rendered,
  /// but during stand-alone game execution this may never be called.
  virtual void AddMainViewsToRender() = 0;

  /// \brief Call this to signal that a game state requested the application to quit.
  ///
  /// xiiGameApplication will shut down when this happens. xiiEditor will stop play-the-game mode when it is running.
  virtual void RequestQuit() = 0;

  /// \brief Returns whether the game state wants to quit the application.
  virtual bool WasQuitRequested() const = 0;

  /// \brief Should be overridden by game states that are only meant as a fallback solution.
  ///
  /// See the implementation for xiiFallbackGameState for details.
  virtual bool IsFallbackGameState() const { return false; }
};
