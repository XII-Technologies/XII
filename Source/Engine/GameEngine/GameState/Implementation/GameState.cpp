/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Components/GamePlay/PlayerStartPointComponent.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
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
  m_bDeactivating = true;
  CancelBackgroundSceneLoading();
  // Join cancelled readers while their component RTTI and plugin code are still loaded.
  m_CancelledSceneLoads.Clear();

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

  if (xiiView* pView = GetMainView(); pView && IsInLoadingScreen())
  {
    const auto  phase   = GetSceneLoadingPhase();
    const char* szPhase = phase == xiiSceneLoadUtility::LoadingPhase::Reading ? "Reading scene..." : "Preloading resources...";
    if (!m_sSceneLoadingError.IsEmpty())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::TopCenter, "Loading", xiiFmt("{}\nPress Escape to exit or select another scene.", m_sSceneLoadingError), xiiColor::OrangeRed);
    else if (phase == xiiSceneLoadUtility::LoadingPhase::Instantiating)
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::TopCenter, "Loading", xiiFmt("Activating scene: {}%%", xiiMath::RoundToInt(m_pBackgroundSceneLoad->GetLoadingProgress() * 100.0f)));
    else if (IsLoadingSceneInBackground() && HasMeasuredSceneLoadingProgress())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::TopCenter, "Loading", xiiFmt("Preloading resources: {}%%", xiiMath::RoundToInt(m_pBackgroundSceneLoad->GetLoadingProgress() * 100.0f)));
    else if (IsLoadingSceneInBackground())
      xiiDebugRenderer::DrawInfoText(*pView, xiiDebugTextPlacement::TopCenter, "Loading", szPhase);
  }
}

xiiView* xiiGameState::GetMainView()
{
  if (!m_pMainWorld)
    return nullptr;

  XII_LOCK(m_pMainWorld->GetReadMarker());
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

xiiWindow* xiiGameState::GetMainWindow()
{
  return static_cast<xiiWindow*>(xiiWindowManager::GetSingleton()->GetWindow(m_hMainWindow));
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
  return m_pLoadingScreenWorld && m_pMainWorld == m_pLoadingScreenWorld;
}

void xiiGameState::CreateWindows()
{
  XII_LOG_BLOCK("CreateWindows");

  xiiUniquePtr<xiiWindow> pMainWindow = CreateMainWindow();
  if (!pMainWindow)
    return;

  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());

  ConfigureMainWindowInputDevices(pMainWindow.Borrow());

  CreateMainView();

  SetupMainView(pOutput->m_pSwapChain, pMainWindow->GetClientAreaSize());

  pMainWindow->SetOutputTarget(std::move(pOutput));

  auto                      pWindowManager = xiiWindowManager::GetSingleton();
  xiiRegisteredWindowHandle hWindowId      = pWindowManager->Register("Game", this, std::move(pMainWindow));
  m_hMainWindow                            = hWindowId;
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

  const auto*                     application = xiiGameApplicationBase::GetGameApplicationBaseInstance();
  xiiRenderPipelineResourceHandle pipeline;
  xiiStringBuilder                pipelineError;
  if (!application || xiiRenderPipelineProfileConfig::ResolveResource({}, &application->GetPlatformProfile(), pipeline, &pipelineError).Failed())
  {
    xiiLog::Error("Cannot create the game view: {}", application ? pipelineError.GetView() : xiiStringView("No game application profile is available."));
    return nullptr;
  }

  XII_LOCK(m_pMainWorld->GetWriteMarker());

  xiiView*              pView              = nullptr;
  xiiRenderWorldModule* pRenderWorldModule = m_pMainWorld->GetOrCreateModule<xiiRenderWorldModule>();
  m_hMainView                              = pRenderWorldModule->CreateView("MainView", pView);

  pView->SetRenderPipelineResource(pipeline);

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

  ++m_uiWorldRevision;
  xiiWorld* pPreviousWorld = m_pMainWorld;
  m_pMainWorld             = pNewMainWorld;

  if (pPreviousWorld != nullptr && !m_hMainView.IsInvalidated())
  {
    XII_LOCK(pPreviousWorld->GetWriteMarker());
    if (xiiRenderWorldModule* pRenderWorldModule = pPreviousWorld->GetModule<xiiRenderWorldModule>())
    {
      pRenderWorldModule->DestroyView(m_hMainView);

      m_hMainView.Invalidate();
    }
  }

  CreateMainView();

  if (auto* pWindow = xiiWindowManager::GetSingleton()->GetWindow(m_hMainWindow))
    if (auto* pOutput = static_cast<xiiWindowOutputTargetGAL*>(pWindow->GetOutputTarget()))
      SetupMainView(pOutput->m_pSwapChain, pWindow->GetClientAreaSize());

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
  const auto* command = xiiCommandLineUtils::GetGlobalInstance();
  if (command->HasOption("-width"))
    windowDescription.m_Resolution.width = xiiMath::Clamp(command->GetIntOption("-width", 1280), 320, 16384);
  if (command->HasOption("-height"))
    windowDescription.m_Resolution.height = xiiMath::Clamp(command->GetIntOption("-height", 720), 200, 16384);
  if (command->HasOption("-display"))
    windowDescription.m_iMonitor = static_cast<xiiInt8>(xiiMath::Clamp(command->GetIntOption("-display", 0), -1, 127));
  const auto mode = command->GetStringOption("-window-mode");
  if (mode == "window")
    windowDescription.m_WindowMode = xiiWindowMode::WindowResizable;
  if (mode == "borderless")
    windowDescription.m_WindowMode = xiiWindowMode::FullscreenBorderlessNativeResolution;
  if (mode == "fullscreen")
    windowDescription.m_WindowMode = xiiWindowMode::FullscreenFixedResolution;
  if (command->HasOption("-window-x") || command->HasOption("-window-y"))
  {
    windowDescription.m_Position.Set(command->GetIntOption("-window-x", 0), command->GetIntOption("-window-y", 0));
    windowDescription.m_bCenterWindowOnDisplay = false;
  }
  const auto instance = command->GetStringOption("-player-instance");
  if (!instance.IsEmpty())
  {
    xiiStringBuilder title;
    title.SetFormat("XII Player [{}] - {}", xiiString(instance).GetLast(8), command->GetStringOption("-role", 0, "standalone"));
    windowDescription.m_Title                = title;
    windowDescription.m_bSetForegroundOnInit = command->GetUIntOption("-player-index", 0) == 0;
  }
  if (command->HasOption("-window-title"))
    windowDescription.m_Title = command->GetStringOption("-window-title");
  windowDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  auto*                   application = xiiGameApplication::GetGameApplicationInstance();
  xiiUniquePtr<xiiWindow> pWindow     = application ? application->TakeStartupWindow() : xiiUniquePtr<xiiWindow>();
  if (pWindow)
  {
    if (pWindow->ApplyWindowSettings(windowDescription).Failed())
      xiiLog::Warning("Cannot apply startup display configuration; retaining the startup window.");
  }
  else
  {
    pWindow = XII_DEFAULT_NEW(xiiWindow);
    if (pWindow->Initialize(windowDescription).Failed())
    {
      xiiLog::Error("Cannot create the game window. Check the display configuration.");
      RequestQuit("Window creation failed");
      return {};
    }
  }

  pWindow->SetTitle(windowDescription.m_Title).IgnoreResult();
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
  xiiPlayerConfig config;
  if (config.Load().Failed())
    xiiLog::Error("Cannot read RuntimeConfigs/Player.ddl. Check the project startup configuration.");
  xiiFileReader      manifestFile;
  xiiProjectManifest manifest;
  if (manifestFile.Open(":project/xiiProject").Succeeded() && manifest.Read(manifestFile).Succeeded())
    out_sScene = manifest.m_sDefaultScene;
  out_sPreloadCollection = config.m_sPreloadCollection;
  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
    out_sPreloadCollection.Clear();
  }
  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-preload"))
    out_sPreloadCollection = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-preload");

  xiiStringBuilder sPreloadCollection = out_sScene;
  sPreloadCollection.ChangeFileExtension("xiiBinCollection");
  if (out_sPreloadCollection.IsEmpty() && xiiFileSystem::ExistsFile(sPreloadCollection))
  {
    out_sPreloadCollection = sPreloadCollection;
  }
}

void xiiGameState::LoadScene(xiiStringView sSceneFile, xiiStringView sPreloadCollection, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  if (m_bInSceneLoadingCallback || m_bDeactivating)
    return;
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

xiiSceneLoadUtility::LoadingPhase xiiGameState::GetSceneLoadingPhase() const
{
  return m_pBackgroundSceneLoad ? m_pBackgroundSceneLoad->GetLoadingPhase() : xiiSceneLoadUtility::LoadingPhase::Ready;
}

xiiResult xiiGameState::TransitionToScene(xiiStringView sSceneFile, xiiStringView sPreloadCollection)
{
  if (sSceneFile.IsEmpty() || m_bInSceneLoadingCallback || m_bDeactivating)
    return XII_FAILURE;
  m_sTargetSceneSpawnPoint.Clear();
  m_TargetSceneSpawnOffset = xiiTransform::MakeIdentity();
  StartBackgroundSceneLoading(sSceneFile, sPreloadCollection);
  m_bTransitionWhenReady = true;
  if (!m_pMainWorld)
    SwitchToLoadingScreen(sSceneFile);
  return XII_SUCCESS;
}

void xiiGameState::SwitchToLoadingScreen(xiiStringView sTargetSceneFile)
{
  if (m_pLoadedWorld && m_pMainWorld == m_pLoadedWorld)
  {
    XII_LOCK(m_pLoadedWorld->GetWriteMarker());
    m_pLoadedWorld->SetWorldSimulationEnabled(false);
  }
  // Detach the view before releasing a previous loading screen.
  auto pPreviousLoadingScreen = std::move(m_pLoadingScreenWorld);
  m_pLoadingScreenWorld       = CreateLoadingScreenWorld(sTargetSceneFile);

  ChangeMainWorld(m_pLoadingScreenWorld.Borrow(), {}, xiiTransform::MakeIdentity());
}

xiiUniquePtr<xiiWorld> xiiGameState::CreateLoadingScreenWorld(xiiStringView sTargetSceneFile)
{
  xiiWorldDescription desc("LoadingScreen");
  return XII_DEFAULT_NEW(xiiWorld, desc);
}

void xiiGameState::StartBackgroundSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollection)
{
  if (m_bInSceneLoadingCallback || m_bDeactivating)
  {
    xiiLog::Error("Scene requests from loading callbacks or deactivation are not supported; enqueue the request for the next frame.");
    return;
  }
  m_bTransitionWhenReady = false;

  if (m_pBackgroundSceneLoad && m_pBackgroundSceneLoad->GetRequestedScene() == sSceneFile &&
      m_pBackgroundSceneLoad->GetRequestedPreloadCollection() == sPreloadCollection)
  {
    const auto state = m_pBackgroundSceneLoad->GetLoadingState();
    if (state == xiiSceneLoadUtility::LoadingState::Ongoing || state == xiiSceneLoadUtility::LoadingState::FinishedSuccessfully)
      return;
  }

  CancelBackgroundSceneLoading();

  m_sSceneLoadingError.Clear();
  m_pBackgroundSceneLoad = XII_DEFAULT_NEW(xiiSceneLoadUtility);
  m_pBackgroundSceneLoad->StartSceneLoading(sSceneFile, sPreloadCollection);
}

void xiiGameState::CancelBackgroundSceneLoading()
{
  if (m_bInSceneLoadingCallback)
    return;
  if (m_pBackgroundSceneLoad)
  {
    m_bInSceneLoadingCallback = true;
    OnBackgroundSceneLoadingCanceled();
    m_bInSceneLoadingCallback = false;

    m_pBackgroundSceneLoad->Cancel();
    if (m_pBackgroundSceneLoad->IsReadTaskFinished())
      m_pBackgroundSceneLoad.Clear();
    else
      m_CancelledSceneLoads.PushBack(std::move(m_pBackgroundSceneLoad));
    m_bTransitionWhenReady = false;
    if (!m_bDeactivating && m_pLoadedWorld && IsInLoadingScreen())
    {
      XII_LOCK(m_pLoadedWorld->GetWriteMarker());
      m_pLoadedWorld->SetWorldSimulationEnabled(true);
      ChangeMainWorld(m_pLoadedWorld.Borrow(), {}, xiiTransform::MakeIdentity());
    }
  }
}

void xiiGameState::UpdateBackgroundSceneLoading()
{
  for (xiiUInt32 i = m_CancelledSceneLoads.GetCount(); i > 0; --i)
    if (m_CancelledSceneLoads[i - 1]->IsReadTaskFinished())
      m_CancelledSceneLoads.RemoveAtAndCopy(i - 1);
  if (m_pBackgroundSceneLoad)
  {
    xiiSceneLoadUtility::LoadingState state = m_pBackgroundSceneLoad->GetLoadingState();

    switch (state)
    {
      case xiiSceneLoadUtility::LoadingState::Cancelled:
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
          m_bInSceneLoadingCallback = true;
          XII_SCOPE_EXIT(m_bInSceneLoadingCallback = false);
          auto             world = m_pBackgroundSceneLoad->RetrieveLoadedScene();
          xiiStringBuilder error;
          xiiResult        activation = XII_FAILURE;
          {
            XII_LOCK(world->GetWriteMarker());
            activation = OnBeforeSceneActivation(*world, error);
          }
          if (activation.Succeeded())
            OnBackgroundSceneLoadingFinished(std::move(world));
          else
            OnBackgroundSceneLoadingFailed(error.IsEmpty() ? xiiStringView("Scene activation was rejected by the application.") : error.GetView());
          m_pBackgroundSceneLoad.Clear();
        }
      }
      break;

      case xiiSceneLoadUtility::LoadingState::Failed:
      {
        m_bInSceneLoadingCallback = true;
        XII_SCOPE_EXIT(m_bInSceneLoadingCallback = false);
        OnBackgroundSceneLoadingFailed(m_pBackgroundSceneLoad->GetLoadingFailureReason());

        m_pBackgroundSceneLoad.Clear();
      }
      break;
    }
  }
}

void xiiGameState::OnBackgroundSceneLoadingFinished(xiiUniquePtr<xiiWorld>&& pWorld)
{
  m_sActiveScene = m_pBackgroundSceneLoad->GetRequestedScene();
  xiiLog::Success("Finished loading scene '{}'.", m_sActiveScene);

  // Keep the old world alive until ChangeMainWorld has detached its view and notified consumers.
  auto pPreviousWorld = std::move(m_pLoadedWorld);
  m_pLoadedWorld      = std::move(pWorld);

  ChangeMainWorld(m_pLoadedWorld.Borrow(), m_sTargetSceneSpawnPoint, m_TargetSceneSpawnOffset);

  m_sTargetSceneSpawnPoint.Clear();

  m_TargetSceneSpawnOffset = xiiTransform::MakeIdentity();
}

void xiiGameState::OnBackgroundSceneLoadingFailed(xiiStringView sReason)
{
  m_sSceneLoadingError = sReason;
  xiiLog::Error("Scene loading failed: {}", sReason);
  if (m_pLoadedWorld && IsInLoadingScreen())
  {
    XII_LOCK(m_pLoadedWorld->GetWriteMarker());
    m_pLoadedWorld->SetWorldSimulationEnabled(true);
    ChangeMainWorld(m_pLoadedWorld.Borrow(), {}, xiiTransform::MakeIdentity());
  }
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
