#pragma once

#include <Core/GameState/GameStateBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

class xiiWindow;
class xiiWindowOutputTargetBase;
class xiiView;
struct xiiActorEvent;
class xiiWindowOutputTargetGAL;
class xiiActor;
class xiiDummyXR;

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

/// \brief xiiGameState is the base class to build custom game logic upon. It works closely together with xiiGameApplication.
///
/// In a typical game there is always exactly one instance of an xiiGameState derived class active.
/// The game state handles custom game logic, which must be handled outside xiiWorld, custom components and scripts.
///
/// For example a custom implementation of xiiGameState may handle how to show a menu, when to switch to
/// another level, how multi-player works, or which player information is transitioned from one level to the next.
/// It's main purpose is to implement high-level game logic.
///
/// xiiGameApplication will loop through all available xiiGameState implementations and ask each available one
/// whether it can handle a certain level. Each game state returns a 'score' how well it can handle the game.
///
/// In a typical game you only have one game state linked into the binary, so in that case there is no reason for
/// such a system. However, in an editor you might have multiple game states available through plugins, but
/// only one can take control.
/// In such a case, each game state may inspect the given world and check whether it is e.g. a single-player
/// or multi-player level, or whether it uses it's own game specific components, and then decide whether
/// it is the best fit for that level.
///
/// \note Do not forget to reflect your derived class, otherwise xiiGameApplication may not find it.
class XII_GAMEENGINE_DLL xiiGameState : public xiiGameStateBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameState, xiiGameStateBase)

protected:
  /// \brief This class cannot be instantiated directly.
  xiiGameState();

public:
  virtual ~xiiGameState();

  /// \brief Returns the active xiiGameState. Only one xiiGameState is allowed to exist.
  static xiiGameState* GetActiveGameState();

  /// \brief Returns the xiiWorld that is currently the active one.
  xiiWorld* GetMainWorld() { return m_pMainWorld; }

  /// \brief When a game state was chosen, it gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected
  /// to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  ///
  /// \param pStartPosition
  /// An optional transform for the 'player object' to start at.
  /// Usually nullptr, but may be set by the editor to relocate or create the player object at the given destination.
  virtual void OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition) override;

  /// \brief Called when the game state is being shut down.
  virtual void OnDeactivation() override;

  /// \brief Has to call xiiRenderLoop::AddMainView for all views that need to be rendered
  virtual void ScheduleRendering() override;

  /// \brief Gives access to the game state's main camera object.
  xiiCamera* GetMainCamera() { return &m_MainCamera; }

  /// \brief Returns the xiiView that is currently the one used for rendering the main output.
  xiiView* GetMainView();

protected:
  /// \brief Creates an actor with a default window (xiiGameStateWindow) adds it to the application
  ///
  /// The base implementation calls CreateMainWindow(), CreateMainOutputTarget() and SetupMainView() to configure the main window.
  virtual void CreateActors();

  /// \brief Adds custom input actions, if necessary.
  /// Unless overridden OnActivation() will call this.
  virtual void ConfigureInputActions();

  /// \brief Overridable function that may create a player object.
  ///
  /// By default called by OnActivation().
  /// The default implementation will search the world for xiiPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the xiiPlayerStartComponent will be used.
  ///
  /// Returns XII_SUCCESS if a prefab was spawned, XII_FAILURE if nothing was done.
  virtual xiiResult SpawnPlayer(const xiiTransform* pStartPosition);

  /// \brief Creates an XR Actor if XR is configured and available for the project.
  xiiUniquePtr<xiiActor> CreateXRActor();

  /// \brief Creates a default main view.
  xiiView* CreateMainView();

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  void ChangeMainWorld(xiiWorld* pNewMainWorld);

  /// \brief Sets up m_MainCamera for first use
  virtual void ConfigureMainCamera();

  /// \brief Override this to modify the default window creation behavior. Called by CreateActors().
  virtual xiiUniquePtr<xiiWindow> CreateMainWindow();

  /// \brief Override this to modify the default output target creation behavior. Called by CreateActors().
  virtual xiiUniquePtr<xiiWindowOutputTargetGAL> CreateMainOutputTarget(xiiWindow* pMainWindow);

  /// \brief Creates a default render view. Unless overridden, OnActivation() will do this for the main window.
  virtual void SetupMainView(xiiGALSwapChainHandle hSwapChain, xiiSizeU32 viewportSize);

  /// \brief Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Called by CreateActors() with the result of CreateMainWindow().
  virtual void ConfigureMainWindowInputDevices(xiiWindow* pWindow);

    static xiiGameState* s_pActiveGameState;

  xiiViewHandle m_hMainView;

  xiiWorld* m_pMainWorld = nullptr;

  xiiCamera                m_MainCamera;
  bool                     m_bStateWantsToQuit  = false;
  bool                     m_bXREnabled         = false;
  bool                     m_bXRRemotingEnabled = false;
  xiiUniquePtr<xiiDummyXR> m_pDummyXR;
};
