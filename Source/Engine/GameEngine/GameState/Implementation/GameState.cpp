/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/GameStateWindow.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

xiiCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

xiiGameState* xiiGameState::s_pActiveGameState = nullptr;

xiiGameState::xiiGameState()
{
  // initialize camera to default values
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

  CreateActors();
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

  xiiRenderWorld::DeleteView(m_hMainView);

  s_pActiveGameState = nullptr;
}

void xiiGameState::AddMainViewsToRender()
{
  if (!m_hMainView.IsInvalidated())
  {
    xiiRenderWorld::AddMainView(m_hMainView);
  }
}

void xiiGameState::RequestQuit()
{
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
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    return pView;
  }

  return nullptr;
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

void xiiGameState::CreateActors()
{
  XII_LOG_BLOCK("CreateActors");

  xiiUniquePtr<xiiWindow> pMainWindow = CreateMainWindow();
  XII_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override xiiGameState::CreateActors().");

  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());

  ConfigureMainWindowInputDevices(pMainWindow.Borrow());
  CreateMainView();
  SetupMainView(pOutput->m_pSwapChain, pMainWindow->GetClientAreaSize());

  {
    // Default flat window
    xiiUniquePtr<xiiActorPluginWindowOwner> pWindowPlugin = XII_DEFAULT_NEW(xiiActorPluginWindowOwner);
    pWindowPlugin->m_pWindow                              = std::move(pMainWindow);
    pWindowPlugin->m_pWindowOutputTarget                  = std::move(pOutput);
    xiiUniquePtr<xiiActor> pActor                         = XII_DEFAULT_NEW(xiiActor, "Main Window", this);
    pActor->AddPlugin(std::move(pWindowPlugin));
    xiiActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void xiiGameState::ConfigureMainWindowInputDevices(xiiWindow* pWindow) {}

void xiiGameState::ConfigureInputActions() {}

void xiiGameState::SetupMainView(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 viewportSize)
{
  xiiView* pView = nullptr;
  if (!xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    xiiLog::Error("Main view is invalid, SetupMainView canceled.");
    return;
  }

  const xiiRenderPipelineProfileConfig* pConfig         = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();
  auto                                  hRenderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
  pView->SetRenderPipelineResource(hRenderPipeline);
  pView->SetSwapChain(pSwapChain);
  pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
  pView->ForceUpdate();
}

xiiView* xiiGameState::CreateMainView()
{
  XII_ASSERT_DEV(m_hMainView.IsInvalidated(), "CreateMainView was already called.");

  XII_LOG_BLOCK("CreateMainView");
  xiiView* pView = nullptr;
  m_hMainView    = xiiRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::MainView);
  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);
  xiiRenderWorld::AddMainView(m_hMainView);

  const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);
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

  xiiWorld* pPrevWorld = m_pMainWorld;

  m_pMainWorld = pNewMainWorld;

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
  }

  OnChangedMainWorld(pPrevWorld, pNewMainWorld, sStartPosition, startPositionOffset);

  // make sure the camera gets re-initialized for the new world
  ConfigureMainCamera();
}

void xiiGameState::OnChangedMainWorld(xiiWorld* pPrevWorld, xiiWorld* pNewWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
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
  if (false)
  {
    xiiHybridArray<xiiScreenInfo, 2> screens;
    xiiScreen::EnumerateScreens(screens).IgnoreResult();
    xiiScreen::PrintScreenInfo(screens);
  }

  xiiStringBuilder sWindowConfig = opt_Window.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWindowConfig.IsEmpty() && !xiiFileSystem::ExistsFile(sWindowConfig))
  {
    xiiLog::Dev("Window Config file does not exist: '{0}'", sWindowConfig);
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

  xiiUniquePtr<xiiGameStateWindow> pWindow = XII_DEFAULT_NEW(xiiGameStateWindow, windowDescription, [] {});
  pWindow->ResetOnClickClose([this]() { this->RequestQuit(); });

  return pWindow;
}

xiiUniquePtr<xiiWindowOutputTargetGAL> xiiGameState::CreateMainOutputTarget(xiiWindow* pMainWindow)
{
  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, [this](xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 vSize) -> void {
    SetupMainView(pSwapChain, vSize);
  });

  xiiGALSwapChainCreationDescription desc;
  desc.m_pWindow               = pMainWindow;
  desc.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  desc.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
  desc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
  desc.m_uiBufferCount         = 2U;
  desc.m_fDefaultDepthValue    = 1.0f;
  desc.m_uiDefaultStencilValue = 0U;

  pOutput->CreateSwapchain(desc);

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
        m_pBackgroundSceneLoad->TickSceneLoading();
        break;

      case xiiSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bTransitionWhenReady)
        {
          OnBackgroundSceneLoadingFinished(m_pBackgroundSceneLoad->RetrieveLoadedScene());
          m_pBackgroundSceneLoad.Clear();
        }
        break;

      case xiiSceneLoadUtility::LoadingState::Failed:
        OnBackgroundSceneLoadingFailed(m_pBackgroundSceneLoad->GetLoadingFailureReason());
        m_pBackgroundSceneLoad.Clear();
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

XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
