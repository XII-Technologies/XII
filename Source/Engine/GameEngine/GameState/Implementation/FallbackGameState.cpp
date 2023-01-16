#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFallbackGameState, 1, xiiRTTIDefaultAllocator<xiiFallbackGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiFallbackGameState::xiiFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}

void xiiFallbackGameState::EnableSceneSelectionMenu(bool bEnable)
{
  m_bEnableSceneSelectionMenu = bEnable;
}

void xiiFallbackGameState::EnableFreeCameras(bool bEnable)
{
  m_bEnableFreeCameras = bEnable;
}

void xiiFallbackGameState::EnableAutoSwitchToLoadedScene(bool bEnable)
{
  m_bAutoSwitchToLoadedScene = bEnable;
}

xiiGameStatePriority xiiFallbackGameState::DeterminePriority(xiiWorld* pWorld) const
{
  return xiiGameStatePriority::Fallback;
}

void xiiFallbackGameState::OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition)
{
  SUPER::OnActivation(pWorld, pStartPosition);

  // if we already have a scene (editor use case), just use that and don't create any other world
  if (pWorld != nullptr)
    return;

  // otherwise we need to load a scene

  SwitchToLoadingScreen();

  if (!xiiFileSystem::ExistsFile(":project/xiiProject"))
  {
    m_bShowMenu = true;

    if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-project"))
      m_State = State::BadProject;
    else
      m_State = State::NoProject;
  }
  else
  {
    xiiStringBuilder sScenePath = GetStartupSceneFile();
    sScenePath.MakeCleanPath();

    if (sScenePath.IsEmpty())
    {
      m_bShowMenu = true;
      m_State     = State::NoScene;
    }
    else if (StartSceneLoading(sScenePath, {}).Failed())
    {
      m_bShowMenu = true;
      m_State     = State::BadScene;
    }
  }
}

void xiiFallbackGameState::OnDeactivation()
{
  CancelSceneLoading();

  SUPER::OnDeactivation();
}

xiiString xiiFallbackGameState::GetStartupSceneFile()
{
  return xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
}

void xiiFallbackGameState::SwitchToLoadingScreen()
{
  m_sTitleOfActiveScene = "Loading Screen";
  m_bIsInLoadingScreen  = true;

  m_pActiveWorld = std::move(CreateLoadingScreenWorld());
  ChangeMainWorld(m_pActiveWorld.Borrow());
}

xiiUniquePtr<xiiWorld> xiiFallbackGameState::CreateLoadingScreenWorld()
{
  xiiWorldDesc desc("LoadingScreen");

  return XII_DEFAULT_NEW(xiiWorld, desc);
}

xiiResult xiiFallbackGameState::StartSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollection)
{
  if (m_pSceneToLoad != nullptr && m_sTitleOfLoadingScene == sSceneFile)
  {
    // already being loaded
    return XII_SUCCESS;
  }

  m_sTitleOfLoadingScene = sSceneFile;

  m_pSceneToLoad = XII_DEFAULT_NEW(xiiSceneLoadUtility);
  m_pSceneToLoad->StartSceneLoading(sSceneFile, sPreloadCollection);

  if (m_pSceneToLoad->GetLoadingState() == xiiSceneLoadUtility::LoadingState::Failed)
  {
    xiiLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
    CancelSceneLoading();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiFallbackGameState::CancelSceneLoading()
{
  m_sTitleOfLoadingScene.Clear();
  m_pSceneToLoad.Clear();
}

bool xiiFallbackGameState::IsLoadingScene() const
{
  return m_pSceneToLoad != nullptr;
}

void xiiFallbackGameState::SwitchToLoadedScene()
{
  XII_ASSERT_DEV(IsLoadingScene(), "Can't switch to loaded scene, if no scene is currently being loaded.");
  XII_ASSERT_DEV(m_pSceneToLoad->GetLoadingState() == xiiSceneLoadUtility::LoadingState::FinishedSuccessfully, "Can't switch to loaded scene before it has finished loading.");

  m_State               = State::Ok;
  m_sTitleOfActiveScene = m_sTitleOfLoadingScene;
  m_pActiveWorld        = m_pSceneToLoad->RetrieveLoadedScene();
  ChangeMainWorld(m_pActiveWorld.Borrow());
  SpawnPlayer(nullptr).IgnoreResult();

  CancelSceneLoading();

  m_bIsInLoadingScreen = false;
}

xiiResult xiiFallbackGameState::SpawnPlayer(const xiiTransform* pStartPosition)
{
  if (SUPER::SpawnPlayer(pStartPosition).Succeeded())
    return XII_SUCCESS;

  if (m_pMainWorld && pStartPosition)
  {
    m_iActiveCameraComponentIndex = -1; // set free camera
    m_MainCamera.LookAt(pStartPosition->m_vPosition, pStartPosition->m_vPosition + pStartPosition->m_qRotation * xiiVec3(1, 0, 0),
                        pStartPosition->m_qRotation * xiiVec3(0, 0, 1));
  }

  return XII_FAILURE;
}

static xiiHybridArray<xiiGameAppInputConfig, 16> g_AllInput;

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  xiiGameAppInputConfig& gacfg = g_AllInput.ExpandAndGetRef();
  gacfg.m_sInputSet            = szInputSet;
  gacfg.m_sInputAction         = szInputAction;
  gacfg.m_sInputSlotTrigger[0] = szKey1;
  gacfg.m_sInputSlotTrigger[1] = szKey2;
  gacfg.m_sInputSlotTrigger[2] = szKey3;
  gacfg.m_bApplyTimeScaling    = true;

  xiiInputActionConfig cfg;

  cfg                     = xiiInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)
    cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)
    cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)
    cfg.m_sInputSlotTrigger[2] = szKey3;

  xiiInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void xiiFallbackGameState::ConfigureInputActions()
{
  g_AllInput.Clear();

  RegisterInputAction("Game", "MoveForwards", xiiInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", xiiInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", xiiInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", xiiInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", xiiInputSlot_KeyE);
  RegisterInputAction("Game", "MoveDown", xiiInputSlot_KeyQ);
  RegisterInputAction("Game", "Run", xiiInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", xiiInputSlot_MouseMoveNegX, xiiInputSlot_KeyLeft);
  RegisterInputAction("Game", "TurnRight", xiiInputSlot_MouseMovePosX, xiiInputSlot_KeyRight);
  RegisterInputAction("Game", "TurnUp", xiiInputSlot_MouseMoveNegY, xiiInputSlot_KeyUp);
  RegisterInputAction("Game", "TurnDown", xiiInputSlot_MouseMovePosY, xiiInputSlot_KeyDown);

  RegisterInputAction("Game", "NextCamera", xiiInputSlot_KeyPageDown);
  RegisterInputAction("Game", "PrevCamera", xiiInputSlot_KeyPageUp);
}

const xiiCameraComponent* xiiFallbackGameState::FindActiveCameraComponent()
{
  if (m_iActiveCameraComponentIndex == -1)
    return nullptr;

  const xiiWorld*                  pWorld   = m_pMainWorld;
  const xiiCameraComponentManager* pManager = pWorld->GetComponentManager<xiiCameraComponentManager>();
  if (pManager == nullptr)
    return nullptr;

  auto itComp = pManager->GetComponents();

  xiiHybridArray<const xiiCameraComponent*, 32> Cameras[xiiCameraUsageHint::ENUM_COUNT];

  // first find all cameras and sort them by usage type
  while (itComp.IsValid())
  {
    const xiiCameraComponent* pComp = itComp;

    if (pComp->IsActive())
    {
      Cameras[pComp->GetUsageHint().GetValue()].PushBack(pComp);
    }

    itComp.Next();
  }

  Cameras[xiiCameraUsageHint::None].Clear();
  Cameras[xiiCameraUsageHint::RenderTarget].Clear();
  Cameras[xiiCameraUsageHint::Culling].Clear();
  Cameras[xiiCameraUsageHint::Shadow].Clear();
  Cameras[xiiCameraUsageHint::Thumbnail].Clear();

  if (m_iActiveCameraComponentIndex == -3)
  {
    // take first camera of a good usage type
    m_iActiveCameraComponentIndex = 0;
  }

  // take last camera (wrap around)
  if (m_iActiveCameraComponentIndex == -2)
  {
    m_iActiveCameraComponentIndex = 0;
    for (xiiUInt32 i = 0; i < xiiCameraUsageHint::ENUM_COUNT; ++i)
    {
      m_iActiveCameraComponentIndex += Cameras[i].GetCount();
    }

    --m_iActiveCameraComponentIndex;
  }

  if (m_iActiveCameraComponentIndex >= 0)
  {
    xiiInt32 offset = m_iActiveCameraComponentIndex;

    // now find the camera by that index
    for (xiiUInt32 i = 0; i < xiiCameraUsageHint::ENUM_COUNT; ++i)
    {
      if (offset < (xiiInt32)Cameras[i].GetCount())
        return Cameras[i][offset];

      offset -= Cameras[i].GetCount();
    }
  }

  // on overflow, reset to free camera
  m_iActiveCameraComponentIndex = -1;
  return nullptr;
}

void xiiFallbackGameState::ProcessInput()
{
  if (IsLoadingScene())
  {
    m_pSceneToLoad->TickSceneLoading();

    switch (m_pSceneToLoad->GetLoadingState())
    {
      case xiiSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bAutoSwitchToLoadedScene)
        {
          SwitchToLoadedScene();
        }
        break;

      case xiiSceneLoadUtility::LoadingState::Failed:
        xiiLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
        CancelSceneLoading();
        break;

      default:
        break;
    }
  }

  if (m_bEnableSceneSelectionMenu)
  {
    if (xiiStringUtils::IsNullOrEmpty(xiiInputManager::GetExclusiveInputSet()) ||
        xiiStringUtils::IsEqual(xiiInputManager::GetExclusiveInputSet(), "xiiPlayer"))
    {
      if (DisplayMenu())
      {
        // prevents the currently active scene from getting any input
        xiiInputManager::SetExclusiveInputSet("xiiPlayer");
      }
      else
      {
        // allows the active scene to retrieve input again
        xiiInputManager::SetExclusiveInputSet("");
      }
    }
  }

  if (m_bEnableFreeCameras)
  {
    XII_LOCK(m_pMainWorld->GetReadMarker());

    if (xiiInputManager::GetInputActionState("Game", "NextCamera") == xiiKeyState::Pressed)
      ++m_iActiveCameraComponentIndex;
    if (xiiInputManager::GetInputActionState("Game", "PrevCamera") == xiiKeyState::Pressed)
      --m_iActiveCameraComponentIndex;

    const xiiCameraComponent* pCamComp = FindActiveCameraComponent();
    if (pCamComp)
    {
      return;
    }

    float fRotateSpeed = 180.0f;
    float fMoveSpeed   = 10.0f;
    float fInput       = 0.0f;

    if (xiiInputManager::GetInputActionState("Game", "Run", &fInput) != xiiKeyState::Up)
      fMoveSpeed *= 10.0f;

    if (xiiInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveLocally(fInput * fMoveSpeed, 0, 0);
    if (xiiInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
    if (xiiInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveLocally(0, -fInput * fMoveSpeed, 0);
    if (xiiInputManager::GetInputActionState("Game", "MoveRight", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveLocally(0, fInput * fMoveSpeed, 0);

    if (xiiInputManager::GetInputActionState("Game", "MoveUp", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, fInput * fMoveSpeed);
    if (xiiInputManager::GetInputActionState("Game", "MoveDown", &fInput) != xiiKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, -fInput * fMoveSpeed);

    if (xiiInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != xiiKeyState::Up)
      m_MainCamera.RotateGlobally(xiiAngle(), xiiAngle(), xiiAngle::Degree(-fRotateSpeed * fInput));
    if (xiiInputManager::GetInputActionState("Game", "TurnRight", &fInput) != xiiKeyState::Up)
      m_MainCamera.RotateGlobally(xiiAngle(), xiiAngle(), xiiAngle::Degree(fRotateSpeed * fInput));
    if (xiiInputManager::GetInputActionState("Game", "TurnUp", &fInput) != xiiKeyState::Up)
      m_MainCamera.RotateLocally(xiiAngle(), xiiAngle::Degree(fRotateSpeed * fInput), xiiAngle());
    if (xiiInputManager::GetInputActionState("Game", "TurnDown", &fInput) != xiiKeyState::Up)
      m_MainCamera.RotateLocally(xiiAngle(), xiiAngle::Degree(-fRotateSpeed * fInput), xiiAngle());
  }
}

void xiiFallbackGameState::AfterWorldUpdate()
{
  XII_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  if (const xiiCameraComponent* pCamComp = FindActiveCameraComponent())
  {
    if (pCamComp->GetCameraMode() != xiiCameraMode::Stereo)
    {
      const xiiGameObject* pOwner    = pCamComp->GetOwner();
      xiiVec3              vPosition = pOwner->GetGlobalPosition();
      xiiVec3              vForward  = pOwner->GetGlobalDirForwards();
      xiiVec3              vUp       = pOwner->GetGlobalDirUp();

      m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
    }
  }
}

void xiiFallbackGameState::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

  if (!xiiFileSystem::ExistsFile(":project/xiiProject"))
    return;

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiFileSystemIterator fsit;
  xiiStringBuilder      sScenePath;

  for (xiiFileSystem::StartSearch(fsit, "", xiiFileSystemIteratorFlags::ReportFilesRecursive);
       fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".xiiScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
#endif
}

bool xiiFallbackGameState::DisplayMenu()
{
  if (IsLoadingScene() || m_pMainWorld == nullptr)
    return false;

  auto pWorld = m_pMainWorld;

  if (m_State == State::NoProject)
  {
    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/xiiProject\"\nto tell xiiPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.xiiScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", xiiColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", xiiGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), xiiColor::Red);

    return false;
  }

  if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyLeftWin) == xiiKeyState::Pressed || xiiInputManager::GetInputSlotState(xiiInputSlot_KeyRightWin) == xiiKeyState::Pressed)
  {
    m_bShowMenu = !m_bShowMenu;
  }

  if (m_State == State::Ok && !m_bShowMenu)
    return false;

  xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Project: '{}'", xiiGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), xiiColor::White);

  if (m_State == State::NoScene)
  {
    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No scene path provided.\n\nUse the command-line argument\n-scene \"Path/To/Scene.xiiScene\"\nto directly load a specific scene.", xiiColor::Orange);
  }
  else if (m_State == State::BadScene)
  {
    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Failed to load scene: '{}'", m_sTitleOfLoadingScene), xiiColor::Red);
  }
  else
  {
    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("Scene: '{}'", m_sTitleOfActiveScene), xiiColor::White);
  }

  if (m_bShowMenu)
  {
    FindAvailableScenes();

    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nSelect scene:\n", xiiColor::White);

    for (xiiUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("> {} <", file), xiiColor::Gold);
      }
      else
      {
        xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", xiiFmt("  {}  ", file), xiiColor::GhostWhite);
      }
    }

    xiiDebugRenderer::DrawInfoText(pWorld, xiiDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", xiiColor::White);

    if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyEscape) == xiiKeyState::Pressed)
    {
      m_bShowMenu = false;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyUp) == xiiKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyDown) == xiiKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (xiiInputManager::GetInputSlotState(xiiInputSlot_KeyReturn) == xiiKeyState::Pressed || xiiInputManager::GetInputSlotState(xiiInputSlot_KeyNumpadEnter) == xiiKeyState::Pressed)
      {
        if (StartSceneLoading(m_AvailableScenes[m_uiSelectedScene], {}).Succeeded())
        {
          m_bShowMenu = false;
        }
        else
        {
          m_bShowMenu = true;
          m_State     = State::BadScene;
        }
      }

      return true;
    }
  }

  return false;
}

bool xiiFallbackGameState::IsInLoadingScreen() const
{
  return m_bIsInLoadingScreen;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);
