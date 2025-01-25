
#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/GameState/GameStateWindow.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRRemotingInterface.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameState, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGameState* xiiGameState::s_pActiveGameState = nullptr;

xiiGameState::xiiGameState() = default;

xiiGameState::~xiiGameState() = default;

xiiGameState* xiiGameState::GetActiveGameState()
{
  return s_pActiveGameState;
}

void xiiGameState::OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition)
{
  s_pActiveGameState = this;

  m_pMainWorld = pWorld;
  {
    ConfigureMainCamera();

    CreateActors();
  }

  ConfigureInputActions();

  SpawnPlayer(pStartPosition).IgnoreResult();
}

void xiiGameState::OnDeactivation()
{
  if (m_bXREnabled)
  {
    m_bXREnabled                 = false;
    xiiXRInterface* pXRInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRInterface>();
    xiiActorManager::GetSingleton()->DestroyAllActors(pXRInterface);
    pXRInterface->Deinitialize();

    if (xiiXRRemotingInterface* pXRRemotingInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRRemotingInterface>())
    {
      if (pXRRemotingInterface->Deinitialize().Failed())
      {
        xiiLog::Error("Failed to deinitialize xiiXRRemotingInterface, make sure all actors are destroyed and xiiXRInterface deinitialized.");
      }
    }

    m_pDummyXR = nullptr;
  }

  xiiRenderWorld::DeleteView(m_hMainView);

  s_pActiveGameState = nullptr;
}

void xiiGameState::ScheduleRendering()
{
  xiiRenderWorld::AddMainView(m_hMainView);
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

xiiUniquePtr<xiiActor> xiiGameState::CreateXRActor()
{
  XII_LOG_BLOCK("CreateXRActor");
  // Init XR
  const xiiXRConfig* pConfig = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiXRConfig>();
  if (!pConfig)
    return nullptr;

  if (!pConfig->m_bEnableXR)
    return nullptr;

  xiiXRInterface* pXRInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRInterface>();
  if (!pXRInterface)
  {
    xiiLog::Warning("No xiiXRInterface interface found. Please load a XR plugin to enable XR. Loading dummyXR interface.");
    m_pDummyXR   = XII_DEFAULT_NEW(xiiDummyXR);
    pXRInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRInterface>();
    XII_ASSERT_DEV(pXRInterface, "Creating dummyXR did not register the xiiXRInterface.");
  }

  xiiXRRemotingInterface* pXRRemotingInterface = xiiSingletonRegistry::GetSingletonInstance<xiiXRRemotingInterface>();
  if (xiiXRRemotingInterface::cvar_XrRemoting)
  {
    if (pXRRemotingInterface)
    {
      if (pXRRemotingInterface->Initialize().Failed())
      {
        xiiLog::Error("xiiXRRemotingInterface could not be initialized. See log for details.");
      }
      else
      {
        m_bXRRemotingEnabled = true;
      }
    }
    else
    {
      xiiLog::Error("No xiiXRRemotingInterface interface found. Please load a XR remoting plugin to enable XR Remoting.");
    }
  }

  if (pXRInterface->Initialize().Failed())
  {
    xiiLog::Error("xiiXRInterface could not be initialized. See log for details.");
    return nullptr;
  }
  m_bXREnabled = true;

  xiiUniquePtr<xiiWindow>                pMainWindow;
  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput;

  if (pXRInterface->SupportsCompanionView())
  {
    // XR Window with added companion window (allows keyboard / mouse input).
    pMainWindow = CreateMainWindow();
    XII_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override xiiGameState::CreateActors().");
    pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
    ConfigureMainWindowInputDevices(pMainWindow.Borrow());
    CreateMainView();
    SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());
  }
  else
  {
    // XR Window (no companion window)
    CreateMainView();
    SetupMainView({}, {});
  }

  if (m_bXRRemotingEnabled)
  {
    if (pXRRemotingInterface->Connect(xiiXRRemotingInterface::cvar_XrRemotingHostName.GetValue().GetData()).Failed())
    {
      xiiLog::Error("Failed to connect XR Remoting.");
    }
  }

  xiiView* pView = nullptr;
  XII_VERIFY(xiiRenderWorld::TryGetView(m_hMainView, pView), "");
  xiiUniquePtr<xiiActor> pXRActor = pXRInterface->CreateActor(pView, xiiGALMSAASampleCount::OneSample, std::move(pMainWindow), std::move(pOutput));
  return std::move(pXRActor);
}

void xiiGameState::CreateActors()
{
  XII_LOG_BLOCK("CreateActors");
  xiiUniquePtr<xiiActor> pXRActor = CreateXRActor();
  if (pXRActor != nullptr)
  {
    xiiActorManager::GetSingleton()->AddActor(std::move(pXRActor));
    return;
  }

  xiiUniquePtr<xiiWindow> pMainWindow = CreateMainWindow();
  XII_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override xiiGameState::CreateActors().");
  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
  ConfigureMainWindowInputDevices(pMainWindow.Borrow());
  CreateMainView();
  SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());

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

void xiiGameState::SetupMainView(xiiGALSwapChainHandle hSwapChain, xiiSizeU32 viewportSize)
{
  xiiView* pView = nullptr;
  if (!xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    xiiLog::Error("Main view is invalid, SetupMainView canceled.");
    return;
  }
  if (m_bXREnabled)
  {
    const xiiXRConfig* pConfig = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiXRConfig>();

    auto renderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sXRRenderPipeline);
    pView->SetRenderPipelineResource(renderPipeline);
    // Render target setup is done by xiiXRInterface::CreateActor
  }
  else
  {
    // Render target setup
    {
      const auto* pConfig        = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>();
      auto        renderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
      pView->SetRenderPipelineResource(renderPipeline);
      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(xiiRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
      pView->ForceUpdate();
    }
  }
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

xiiResult xiiGameState::SpawnPlayer(const xiiTransform* pStartPosition)
{
  if (m_pMainWorld == nullptr)
    return XII_FAILURE;

  XII_LOCK(m_pMainWorld->GetWriteMarker());

  xiiPlayerStartPointComponentManager* pMan = m_pMainWorld->GetComponentManager<xiiPlayerStartPointComponentManager>();
  if (pMan == nullptr)
    return XII_FAILURE;

  for (auto it = pMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      xiiResourceLock<xiiPrefabResource> pPrefab(it->GetPlayerPrefab(), xiiResourceAcquireMode::BlockTillLoaded);

      if (pPrefab.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        const xiiUInt16 uiTeamID = it->GetOwner()->GetTeamID();
        xiiTransform    startPos = it->GetOwner()->GetGlobalTransform();

        if (pStartPosition)
        {
          startPos = *pStartPosition;
          startPos.m_vScale.Set(1.0f);
          startPos.m_vPosition.z += 1.0f; // do not spawn player prefabs on the ground, they may not have their origin there
        }

        xiiPrefabInstantiationOptions options;
        options.m_pOverrideTeamID = &uiTeamID;

        pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, options, &(it->m_Parameters));

        return XII_SUCCESS;
      }
    }
  }

  return XII_FAILURE;
}

void xiiGameState::ChangeMainWorld(xiiWorld* pNewMainWorld)
{
  m_pMainWorld = pNewMainWorld;

  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
  }
}

void xiiGameState::ConfigureMainCamera()
{
  xiiVec3 vCameraPos = xiiVec3(0.0f, 0.0f, 0.0f);

  xiiCoordinateSystem coordSys;

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
  if (m_MainCamera.GetCameraMode() != xiiCameraMode::Stereo)
  {
    m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
    m_MainCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  }
}

xiiCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

xiiUniquePtr<xiiWindow> xiiGameState::CreateMainWindow()
{
  if (false)
  {
    xiiHybridArray<xiiScreenInfo, 2> screens;
    xiiScreen::EnumerateScreens(screens).IgnoreResult();
    xiiScreen::PrintScreenInfo(screens);
  }

  xiiStringBuilder sWndCfg = opt_Window.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWndCfg.IsEmpty() && !xiiFileSystem::ExistsFile(sWndCfg))
  {
    xiiLog::Dev("Window Config file does not exist: '{0}'", sWndCfg);
    sWndCfg.Clear();
  }

  if (sWndCfg.IsEmpty())
  {
    const xiiStringView sCfgAppData = ":appdata/RuntimeConfigs/Window.ddl";
    const xiiStringView sCfgProject = ":project/RuntimeConfigs/Window.ddl";

    if (xiiFileSystem::ExistsFile(sCfgAppData))
      sWndCfg = sCfgAppData;
    else
      sWndCfg = sCfgProject;
  }

  xiiWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg).IgnoreResult();

  xiiUniquePtr<xiiGameStateWindow> pWindow = XII_DEFAULT_NEW(xiiGameStateWindow, wndDesc, [] {});
  pWindow->ResetOnClickClose([this]() { this->RequestQuit(); });
  if (pWindow->GetInputDevice())
    pWindow->GetInputDevice()->SetMouseSpeed(xiiVec2(0.002f));

  return pWindow;
}

xiiUniquePtr<xiiWindowOutputTargetGAL> xiiGameState::CreateMainOutputTarget(xiiWindow* pMainWindow)
{
  xiiUniquePtr<xiiWindowOutputTargetGAL> pOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, [this](xiiGALSwapChainHandle hSwapChain, xiiSizeU32 size) {
    SetupMainView(hSwapChain, size);
  });

  xiiGALSwapChainCreationDescription desc;
  desc.m_pWindow               = pMainWindow;
  desc.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  desc.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget;
  desc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
  desc.m_uiBufferCount         = 2U;
  desc.m_fDefaultDepthValue    = 1.0f;
  desc.m_uiDefaultStencilValue = 0U;

  pOutput->CreateSwapchain(desc);

  return pOutput;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
