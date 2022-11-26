#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFallbackGameState, 1, xiiRTTIDefaultAllocator<xiiFallbackGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiFallbackGameState::xiiFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}



xiiGameStatePriority xiiFallbackGameState::DeterminePriority(xiiWorld* pWorld) const
{
  if (pWorld == nullptr)
    return

      xiiGameStatePriority::None;

  return

    xiiGameStatePriority::Fallback;
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

  // if ( !xiiFileSystem::ExistsFile( ":project/InputConfig.ddl" ) )
  {
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

    // if ( !g_AllInput.IsEmpty() )
    //{
    //	xiiFileWriter file;
    //	if ( file.Open( ":project/InputConfig.ddl" ).Succeeded() )
    //	{
    //		xiiGameAppInputConfig::WriteToDDL( file, g_AllInput );
    //	}
    //}
  }
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



XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);
