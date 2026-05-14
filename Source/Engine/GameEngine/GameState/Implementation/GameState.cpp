/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Components/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

xiiCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGameState* xiiGameState::s_pActiveGameState = nullptr;

xiiGameState::xiiGameState()
{
  // Initialize camera to default values.
  m_MainCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  m_MainCamera.LookAt(xiiVec3::MakeZero(), xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
}

xiiGameState::~xiiGameState() = default;

xiiGameState* xiiGameState::GetActiveGameState()
{
  return s_pActiveGameState;
}

void xiiGameState::OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  s_pActiveGameState = this;

  CreateWindows();
  ConfigureInputActions();

  if (pWorld)
  {
    ChangeMainWorld(pWorld, sStartPosition, startPositionOffset);
  }
  else
  {
    xiiString sSceneFile;
    xiiString sPreloadCollection;
    GetStartupOptions(sSceneFile, sPreloadCollection);

    if (!sSceneFile.IsEmpty())
    {
      LoadScene(sSceneFile, sPreloadCollection, sStartPosition, startPositionOffset);
    }
  }
}

void xiiGameState::OnDeactivation()
{
  CancelBackgroundSceneLoading();

  if (m_pMainWorld != nullptr && !m_hMainView.IsInvalidated())
  {
    XII_LOCK(m_pMainWorld->GetWriteMarker());

    xiiRenderWorldModule* pRenderWorldModule = m_pMainWorld->GetModule<xiiRenderWorldModule>();

    pRenderWorldModule->DestroyView(m_hMainView);

    m_hMainView.Invalidate();
  }

  s_pActiveGameState = nullptr;
}

void xiiGameState::RequestQuit(xiiStringView sRequestedBy)
{
  XII_IGNORE_UNUSED(sRequestedBy);

  m_bStateWantsToQuit = true;
}

bool xiiGameState::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}

void xiiGameState::ProcessInput()
{
  UpdateBackgroundSceneLoading();
}

xiiView* xiiGameState::GetMainView()
{
  if (!m_pMainWorld)
    return nullptr;

  xiiView* pView = nullptr;

  if (const xiiRenderWorldModule* pRenderWorldModule = m_pMainWorld->GetModuleReadOnly<xiiRenderWorldModule>())
  {
    if (pRenderWorldModule->TryGetView(m_hMainView, pView))
    {
      return pView;
    }
  }

  return pView;
}

bool xiiGameState::IsLoadingSceneInBackground(float* out_pProgress) const
{
  if (out_pProgress)
  {
    *out_pProgress = 0.0f;

    if (m_pBackgroundSceneLoad != nullptr)
    {
      *out_pProgress = m_pBackgroundSceneLoad->GetLoadingProgress();

      auto state = m_pBackgroundSceneLoad->GetLoadingState();

      if (state != xiiSceneLoadUtility::LoadingState::FinishedSuccessfully)
      {
        *out_pProgress = xiiMath::Min(*out_pProgress, 0.99f);
      }
    }
  }

  return m_pBackgroundSceneLoad != nullptr;
}

bool xiiGameState::IsInLoadingScreen() const
{
  return m_pMainWorld == m_pLoadingScreenWorld;
}

void xiiGameState::CreateWindows()
{
  XII_LOG_BLOCK("CreateWindows");

  xiiUniquePtr<xiiWindow> pMainWindow = CreateMainWindow();
  XII_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override xiiGameState::CreateWindows().");

  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());

  ConfigureMainWindowInputDevices(pMainWindow.Borrow());

  CreateMainView();

  SetupMainView(pOutput->m_pSwapChain, pMainWindow->GetClientAreaSize());

  pMainWindow->SetOutputTarget(std::move(pOutput));

  auto                      pWindowManager = xiiWindowManager::GetSingleton();
  xiiRegisteredWindowHandle hWindowId      = pWindowManager->Register("Game", this, std::move(pMainWindow));
  XII_IGNORE_UNUSED(hWindowId);
}

void xiiGameState::ConfigureMainWindowInputDevices(xiiWindow* pWindow) {}

void xiiGameState::ConfigureInputActions() {}

void xiiGameState::SetupMainView(xiiGALSwapChain* pSwapChain, xiiSizeU32 viewportSize)
{
  if (xiiView* pView = GetMainView())
  {
    pView->SetSwapChain(pSwapChain);
    pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
  }
}

xiiView* xiiGameState::CreateMainView()
{
  XII_ASSERT_DEV(m_hMainView.IsInvalidated(), "CreateMainView was already called.");

  XII_LOG_BLOCK("CreateMainView");

  if (m_pMainWorld == nullptr)
    return nullptr;

  XII_LOCK(m_pMainWorld->GetWriteMarker());

  xiiView*              pView              = nullptr;
  xiiRenderWorldModule* pRenderWorldModule = m_pMainWorld->GetOrCreateModule<xiiRenderWorldModule>();
  m_hMainView                              = pRenderWorldModule->CreateView("MainView", pView);

  pView->SetCameraUsageHint(xiiCameraUsageHint::MainView);
  pView->SetCamera(&m_MainCamera);

  const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  pView->m_ExcludeTags.Set(tagEditor); // Exclude all editor objects from rendering in proper game views.

  return pView;
}

xiiResult xiiGameState::SpawnPlayer(xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  if (m_pMainWorld == nullptr)
    return XII_FAILURE;

  XII_LOCK(m_pMainWorld->GetWriteMarker());

  xiiPlayerStartPointComponentManager* pPlayerStartComponentManager = m_pMainWorld->GetComponentManager<xiiPlayerStartPointComponentManager>();
  if (pPlayerStartComponentManager == nullptr)
    return XII_FAILURE;

  xiiPlayerStartPointComponent* pBestPlayerStartComponent = nullptr;

  for (auto it = pPlayerStartComponentManager->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      if (pBestPlayerStartComponent == nullptr)
      {
        // take the first one, no matter what
        pBestPlayerStartComponent = it;
      }
      else if (it->GetOwner()->GetName().IsEqual_NoCase(sStartPosition))
      {
        // if we find one by exact name match, take that one
        pBestPlayerStartComponent = it;
      }
      else if (!pBestPlayerStartComponent->GetOwner()->GetName().IsEqual_NoCase(sStartPosition) && it->GetOwner()->GetName().IsEmpty())
      {
        // if the name of the best one isn't identical to the searched name, yet
        // and this one is nameless, prefer the nameless one
        pBestPlayerStartComponent = it;
      }
    }
  }

  if (pBestPlayerStartComponent)
  {
    xiiResourceLock<xiiPrefabResource> pPrefab(pBestPlayerStartComponent->GetPlayerPrefab(), xiiResourceAcquireMode::BlockTillLoaded);

    if (pPrefab.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      const xiiUInt16 uiTeamID      = pBestPlayerStartComponent->GetOwner()->GetTeamID();
      xiiTransform    startPosition = xiiTransform::MakeGlobalTransform(pBestPlayerStartComponent->GetOwner()->GetGlobalTransform(), startPositionOffset);

      if (sStartPosition.IsEqual_NoCase("GlobalOverride"))
      {
        startPosition = startPositionOffset;
      }

      startPosition.m_vScale.Set(1.0f);

      xiiPrefabInstantiationOptions options;
      options.m_pOverrideTeamID = &uiTeamID;

      pPrefab->InstantiatePrefab(*m_pMainWorld, startPosition, options, &(pBestPlayerStartComponent->m_Parameters));

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

void xiiGameState::ChangeMainWorld(xiiWorld* pNewMainWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  if (m_pMainWorld == pNewMainWorld)
    return;

  xiiWorld* pPreviousWorld = m_pMainWorld;
  m_pMainWorld             = pNewMainWorld;

  if (pPreviousWorld != nullptr && !m_hMainView.IsInvalidated())
  {
    if (xiiRenderWorldModule* pRenderWorldModule = pPreviousWorld->GetModule<xiiRenderWorldModule>())
    {
      pRenderWorldModule->DestroyView(m_hMainView);

      m_hMainView.Invalidate();
    }
  }

  CreateMainView();

  OnChangedMainWorld(pPreviousWorld, pNewMainWorld, sStartPosition, startPositionOffset);

  // make sure the camera gets re-initialized for the new world
  ConfigureMainCamera();
}

void xiiGameState::OnChangedMainWorld(xiiWorld* pPreviousWorld, xiiWorld* pNewWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  if (pNewWorld != m_pLoadingScreenWorld)
  {
    // can get rid of the loading screen world, or we could also keep it around for later, if that has any use
    m_pLoadingScreenWorld.Clear();

    SpawnPlayer(sStartPosition, startPositionOffset).IgnoreResult();
  }
}

void xiiGameState::ConfigureMainCamera()
{
  if (m_MainCamera.GetCameraMode() == xiiCameraMode::Stereo)
  {
    // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
    return;
  }

  if (const xiiWorld* pConstWorld = m_pMainWorld)
  {
    XII_LOCK(pConstWorld->GetReadMarker());

    const xiiCameraComponentManager* pManager = pConstWorld->GetComponentManager<xiiCameraComponentManager>();
    if (pManager != nullptr)
    {
      for (auto itCameraComponent = pManager->GetComponents(); itCameraComponent.IsValid(); itCameraComponent.Next())
      {
        const xiiCameraComponent* pCameraComponent = itCameraComponent;

        if (pCameraComponent->IsActive() && pCameraComponent->GetUsageHint() == xiiCameraUsageHint::MainView)
        {
          xiiVec3 vCameraPosition = pCameraComponent->GetOwner()->GetGlobalPosition();

          xiiCoordinateSystem coordinateSystem;
          coordinateSystem.m_vForwardDir = pCameraComponent->GetOwner()->GetGlobalDirForwards();
          coordinateSystem.m_vRightDir   = pCameraComponent->GetOwner()->GetGlobalDirRight();
          coordinateSystem.m_vUpDir      = pCameraComponent->GetOwner()->GetGlobalDirUp();

          // update the camera position
          // camera options (FOV etc) are already set by xiiCameraComponentManager on demand
          m_MainCamera.LookAt(vCameraPosition, vCameraPosition + coordinateSystem.m_vForwardDir, coordinateSystem.m_vUpDir);
          return;
        }
      }
    }
  }
}

xiiUniquePtr<xiiWindow> xiiGameState::CreateMainWindow()
{
  xiiStringBuilder sWindowConfig = opt_Window.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWindowConfig.IsEmpty() && !xiiFileSystem::ExistsFile(sWindowConfig))
  {
    xiiLog::Dev("Window configuration file does not exist: '{0}'", sWindowConfig);
    sWindowConfig.Clear();
  }

  if (sWindowConfig.IsEmpty())
  {
    const xiiStringView sCfgAppData = ":appdata/RuntimeConfigs/Window.ddl";
    const xiiStringView sCfgProject = ":project/RuntimeConfigs/Window.ddl";

    if (xiiFileSystem::ExistsFile(sCfgAppData))
    {
      sWindowConfig = sCfgAppData;
    }
    else
    {
      sWindowConfig = sCfgProject;
    }
  }

  xiiWindowCreationDescription windowDescription;
  windowDescription.LoadFromDDL(sWindowConfig).IgnoreResult();
  windowDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  xiiUniquePtr<xiiWindow> pWindow = XII_DEFAULT_NEW(xiiWindow);
  pWindow->Initialize(windowDescription).AssertSuccess("Failed to create window.");

  pWindow->GetWindowEvents().AddEventHandler(xiiMakeDelegate(&xiiGameState::OnWindowEvent, this));

  return pWindow;
}

xiiUniquePtr<xiiWindowOutputTargetGAL> xiiGameState::CreateMainOutputTarget(xiiWindow* pMainWindow)
{
  xiiGALSwapChainCreationDescription description;
  description.m_pWindow               = pMainWindow;
  description.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  description.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
  description.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
  description.m_uiBufferCount         = 2U;
  description.m_fDefaultDepthValue    = 1.0f;
  description.m_uiDefaultStencilValue = 0U;

  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, description, [this](xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 vSize) -> void {
    SetupMainView(pSwapChain, vSize);
  });

  return pOutput;
}

void xiiGameState::GetStartupOptions(xiiString& out_sScene, xiiString& out_sPreloadCollection)
{
  out_sScene = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");

  xiiStringBuilder sPreloadCollection = out_sScene;
  sPreloadCollection.ChangeFileExtension("xiiBinCollection");
  if (xiiFileSystem::ExistsFile(sPreloadCollection))
  {
    out_sPreloadCollection = sPreloadCollection;
  }
}

void xiiGameState::LoadScene(xiiStringView sSceneFile, xiiStringView sPreloadCollection, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  m_sTargetSceneSpawnPoint = sStartPosition;
  m_TargetSceneSpawnOffset = startPositionOffset;

  StartBackgroundSceneLoading(sSceneFile, sPreloadCollection);
  m_bTransitionWhenReady = true;

  auto state = m_pBackgroundSceneLoad->GetLoadingState();
  XII_ASSERT_DEBUG(state != xiiSceneLoadUtility::LoadingState::FinishedAndRetrieved, "Scene already loaded and retrieved.");

  if (state != xiiSceneLoadUtility::LoadingState::FinishedSuccessfully)
  {
    // switch to loading screen only if we can't immediately switch to the target scene
    SwitchToLoadingScreen(sSceneFile);
  }
}

void xiiGameState::SwitchToLoadingScreen(xiiStringView sTargetSceneFile)
{
  m_pLoadingScreenWorld = CreateLoadingScreenWorld(sTargetSceneFile);

  ChangeMainWorld(m_pLoadingScreenWorld.Borrow(), {}, xiiTransform::MakeIdentity());
}

xiiUniquePtr<xiiWorld> xiiGameState::CreateLoadingScreenWorld(xiiStringView sTargetSceneFile)
{
  xiiWorldDesc desc("LoadingScreen");
  return XII_DEFAULT_NEW(xiiWorld, desc);
}

void xiiGameState::StartBackgroundSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollection)
{
  m_bTransitionWhenReady = false;

  if ((m_pBackgroundSceneLoad != nullptr) && (m_pBackgroundSceneLoad->GetRequestedScene() == sSceneFile))
  {
    // already being loaded
    return;
  }

  CancelBackgroundSceneLoading();

  m_pBackgroundSceneLoad = XII_DEFAULT_NEW(xiiSceneLoadUtility);
  m_pBackgroundSceneLoad->StartSceneLoading(sSceneFile, sPreloadCollection);
}

void xiiGameState::CancelBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    OnBackgroundSceneLoadingCanceled();

    m_pBackgroundSceneLoad.Clear();
  }
}

void xiiGameState::UpdateBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    xiiSceneLoadUtility::LoadingState state = m_pBackgroundSceneLoad->GetLoadingState();

    switch (state)
    {
      case xiiSceneLoadUtility::LoadingState::FinishedAndRetrieved:
        return;

      case xiiSceneLoadUtility::LoadingState::NotStarted:
      case xiiSceneLoadUtility::LoadingState::Ongoing:
      {
        m_pBackgroundSceneLoad->TickSceneLoading();
      }
      break;

      case xiiSceneLoadUtility::LoadingState::FinishedSuccessfully:
      {
        if (m_bTransitionWhenReady)
        {
          OnBackgroundSceneLoadingFinished(m_pBackgroundSceneLoad->RetrieveLoadedScene());

          m_pBackgroundSceneLoad.Clear();
        }
      }
      break;

      case xiiSceneLoadUtility::LoadingState::Failed:
      {
        OnBackgroundSceneLoadingFailed(m_pBackgroundSceneLoad->GetLoadingFailureReason());

        m_pBackgroundSceneLoad.Clear();
      }
      break;
    }
  }
}

void xiiGameState::OnBackgroundSceneLoadingFinished(xiiUniquePtr<xiiWorld>&& pWorld)
{
  xiiLog::Success("Finished loading scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());

  m_pLoadedWorld = std::move(pWorld);

  ChangeMainWorld(m_pLoadedWorld.Borrow(), m_sTargetSceneSpawnPoint, m_TargetSceneSpawnOffset);

  m_sTargetSceneSpawnPoint.Clear();

  m_TargetSceneSpawnOffset = xiiTransform::MakeIdentity();
}

void xiiGameState::OnBackgroundSceneLoadingFailed(xiiStringView sReason)
{
  xiiLog::Error("Scene loading failed: {}", sReason);
}

void xiiGameState::OnBackgroundSceneLoadingCanceled()
{
  xiiLog::Dev("Cancelled background loading of scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());
}

void xiiGameState::OnWindowEvent(const xiiWindowEvent& e)
{
  if (e.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
  {
    // Forward the event to the game state, so that it can decide what to do with it. For example, it may want to show a confirmation dialog before actually quitting.
    RequestQuit("Game");
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
