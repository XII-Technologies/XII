/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/PlayerMessages.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

xiiGameApplication*                                                             xiiGameApplication::s_pGameApplicationInstance = nullptr;
xiiDelegate<xiiSharedPtr<xiiGALDevice>(const xiiGALDeviceCreationDescription&)> xiiGameApplication::s_DefaultDeviceCreator;

xiiCVarBool xiiGameApplication::cvar_AppVSync("App.VSync", true, xiiCVarFlags::Save, "Compatibility control for runtime presentation settings. Updates VSync in RuntimeConfigs/Presentation.ddl; the reflected file takes precedence over legacy saved CVar values.");

static xiiCVarFloat cvar_FrameTimeWarningMS("App.FrameTimeWarningMS", 0.0f, xiiCVarFlags::Default, "Log frames exceeding this measured wall-clock duration in milliseconds. Zero disables warnings; intended for temporary hitch diagnosis.");

xiiGameApplication::xiiGameApplication(xiiStringView sAppName, xiiStringView sProjectPath /*= {}*/) :
  xiiGameApplicationBase(sAppName), m_sAppProjectPath(sProjectPath)
{
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested        = false;

  m_pConsole = XII_DEFAULT_NEW(xiiQuakeConsole);
  xiiConsole::SetMainConsole(m_pConsole.Borrow());

  m_CVarChangeSubscriptionID = cvar_AppVSync.m_CVarEvents.AddEventHandler(xiiMakeDelegate(&xiiGameApplication::OnVSyncChanged, this));
  m_Settings.m_SettingsChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiGameApplication::OnSettingsChanged, this), m_SettingsSubscription);
  if (auto* cap = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit")))
    m_FrameLimitSubscriptionID = cap->m_CVarEvents.AddEventHandler([this, cap](const xiiCVarEvent& event) {
      if (event.m_EventType != xiiCVarEvent::ValueChanged || !m_bSettingsInitialized)
        return;
      const auto value = static_cast<xiiUInt32>(xiiMath::Clamp(cap->GetValue(), 0, 1000));
      if (m_Settings.m_uiFrameLimit != value)
      {
        m_Settings.m_uiFrameLimit = value;
        m_Settings.NotifyChanged();
      }
    });
}

xiiGameApplication::~xiiGameApplication()
{
  s_pGameApplicationInstance = nullptr;

  cvar_AppVSync.m_CVarEvents.RemoveEventHandler(m_CVarChangeSubscriptionID);
  if (auto* cap = xiiCVar::FindCVarByName("App.FrameLimit"))
    cap->m_CVarEvents.RemoveEventHandler(m_FrameLimitSubscriptionID);
  if (xiiConsole::GetMainConsole() == m_pConsole.Borrow())
    xiiConsole::SetMainConsole(nullptr);
}

// static
void xiiGameApplication::SetOverrideDefaultDeviceCreator(xiiDelegate<xiiSharedPtr<xiiGALDevice>(const xiiGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
}

void xiiGameApplication::ReinitializeInputConfig()
{
  Init_ConfigureInput();
}

xiiString xiiGameApplication::FindProjectDirectory() const
{
  XII_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built-in project directory passed to the xiiGameApplication constructor, or m_sAppProjectPath must be set manually before doing project setup, or xiiGameApplication::FindProjectDirectory() must be overridden.");

  if (xiiPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the SDK special directory
  {
    xiiStringBuilder sPathRelativeToSDK(m_sAppProjectPath);

    if (!sPathRelativeToSDK.StartsWith_NoCase(">sdk/"))
    {
      sPathRelativeToSDK.Prepend(">sdk/");
    }

    xiiStringBuilder sAbsolutePathToSDK;
    if (xiiFileSystem::ResolveSpecialDirectory(sPathRelativeToSDK, sAbsolutePathToSDK).Succeeded())
    {
      if (xiiOSFile::ExistsDirectory(sAbsolutePathToSDK))
        return sAbsolutePathToSDK;
    }
  }

  xiiStringBuilder sResult;
  if (xiiFileSystem::FindFolderWithSubPath(sResult, xiiOSFile::GetApplicationDirectory(), m_sAppProjectPath).Failed())
  {
    xiiLog::Error("Could not find the project directory.");
  }

  return sResult;
}

void xiiGameApplication::Run_WorldUpdateAndRender()
{
  XII_PROFILE_SCOPE("Run_WorldUpdateAndRender");

  RenderConsole();
  Run_BeforeWorldUpdate();

  xiiTaskGroupID updateWorldsTaskID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
  for (xiiUInt32 i = 0; i < xiiWorld::GetWorldCount(); ++i)
  {
    xiiWorld* pWorld = xiiWorld::GetWorld(i);

    if (pWorld != nullptr && pWorld->GetWorldSimulationEnabled())
    {
      xiiTaskSystem::AddTaskToGroup(updateWorldsTaskID, pWorld->GetUpdateTask());
    }
  }
  xiiTaskSystem::StartTaskGroup(updateWorldsTaskID);
  xiiTaskSystem::WaitForGroup(updateWorldsTaskID);

  Run_AfterWorldUpdate();

  Run_UpdatePlugins();
}

void xiiGameApplication::Run_PresentImage()
{
  auto pWindowManager = xiiWindowManager::GetSingleton();

  xiiTemporaryHybridArray<xiiRegisteredWindowHandle, 8> windows;
  pWindowManager->GetRegistered(windows);

  bool bExecutedFrameCapture = false;
  for (xiiRegisteredWindowHandle hWindow : windows)
  {
    xiiWindowBase* pWindow = pWindowManager->GetWindow(hWindow);
    if (pWindow == nullptr)
      continue;

    if (xiiWindowOutputTargetBase* pOutput = pWindow->GetOutputTarget())
    {
      // if we have multiple actors, append the actor name to each screenshot.
      xiiStringBuilder sContext;
      if (windows.GetCount() > 1)
      {
        sContext.Append(" - ", pWindowManager->GetName(hWindow));
      }

      ExecuteTakeScreenshot(pOutput, sContext);

      if (!bExecutedFrameCapture)
      {
        ExecuteFrameCapture(pWindow->GetNativeWindowHandle(), sContext);
        bExecutedFrameCapture = true;
      }

      XII_PROFILE_SCOPE("PresentImage");
      pOutput->PresentImage();
    }
  }
}

void xiiGameApplication::Run_FinishFrame()
{
  SUPER::Run_FinishFrame();
  const float fThreshold = cvar_FrameTimeWarningMS.GetValue();
  if (xiiMath::IsFinite(fThreshold) && fThreshold > 0.0f && m_FrameTime.GetMilliseconds() > fThreshold)
    xiiLog::Warning("Frame took {0} ms (App.FrameTimeWarningMS = {1}).", m_FrameTime.GetMilliseconds(), fThreshold);
}

void xiiGameApplication::RenderConsole()
{
  XII_PROFILE_SCOPE("RenderConsole");

  if (!m_bShowConsole || !m_pConsole)
    return;

  if (xiiGameState* pGameState = xiiDynamicCast<xiiGameState*>(m_pGameState.Borrow()))
  {
    if (xiiWorld* pMainWorld = pGameState->GetMainWorld())
    {
      XII_LOCK(pMainWorld->GetWriteMarker());
      xiiRenderWorldModule* pRenderWorldModule = pMainWorld->GetOrCreateModule<xiiRenderWorldModule>();

      if (const xiiView* pView = pRenderWorldModule->GetViewByUsageHint(xiiCameraUsageHint::MainView))
      {
        const float fViewWidth             = pView->GetViewport().width;
        const float fViewHeight            = pView->GetViewport().height;
        const float fGlyphWidth            = xiiDebugRenderer::GetTextGlyphWidth();
        const float fLineHeight            = xiiDebugRenderer::GetTextLineHeight();
        const float fConsoleHeight         = (fViewHeight / 2.0f);
        const float fBorderWidth           = 3.0f;
        const float fConsoleTextAreaHeight = fConsoleHeight - fLineHeight - (2.0f * fBorderWidth);

        const xiiInt32 iTextHeight = (xiiInt32)fLineHeight;
        const xiiInt32 iTextLeft   = (xiiInt32)(fBorderWidth);

        {
          xiiColor backgroundColor(0.0f, 0.0f, 0.0f, 0.7f);
          xiiDebugRenderer::Draw2DRectangle(*pView, xiiRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

          xiiColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
          xiiDebugRenderer::Draw2DRectangle(*pView, xiiRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
          xiiDebugRenderer::Draw2DRectangle(*pView, xiiRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fLineHeight), 0.0f, foregroundColor);
        }

        {
          XII_LOCK(m_pConsole->GetMutex());

          auto& consoleStrings = m_pConsole->GetConsoleStrings();

          xiiUInt32 uiNumConsoleLines = (xiiUInt32)(xiiMath::Ceil(fConsoleTextAreaHeight / fLineHeight));
          xiiInt32  iFirstLinePos     = (xiiInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
          xiiInt32  uiFirstLine       = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
          xiiInt32  uiSkippedLines    = xiiMath::Max(uiFirstLine - (xiiInt32)consoleStrings.GetCount() + 1, 0);

          for (xiiUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
          {
            auto& consoleString = consoleStrings[uiFirstLine - i];
            xiiDebugRenderer::Draw2DText(*pView, consoleString.m_sText.GetData(), xiiVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
          }

          xiiDebugRenderer::Draw2DText(*pView, m_pConsole->GetInputLine(), xiiVec2I32(iTextLeft, (xiiInt32)(fConsoleTextAreaHeight + fBorderWidth + (fLineHeight * 0.5f))), xiiColor::White, 16, xiiDebugTextHAlign::Default, xiiDebugTextVAlign::Center);

          if (xiiMath::Fraction(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
          {
            const float fCaretPosition = (float)m_pConsole->GetCaretPosition();
            const float fCaretX        = fBorderWidth + (fCaretPosition + 0.5f) * fGlyphWidth;
            const float fCaretY        = fConsoleTextAreaHeight + fBorderWidth + 1.0f;
            xiiColor    caretColor(1.0f, 1.0f, 1.0f, 0.5f);
            xiiDebugRenderer::Draw2DRectangle(*pView, xiiRectFloat(fCaretX, fCaretY, 2.0f, fLineHeight - 2.0f), 0.0f, caretColor);
          }
        }
      }
    }
  }
}

void xiiGameApplication::OnVSyncChanged(const xiiCVarEvent& e)
{
  if (e.m_EventType != xiiCVarEvent::ValueChanged || !m_bSettingsInitialized || m_Settings.m_bVSync == cvar_AppVSync.GetValue())
    return;
  m_Settings.m_bVSync = cvar_AppVSync.GetValue();
  m_Settings.NotifyChanged();
}

void xiiGameApplication::Init_ConfigureCVars()
{
  SUPER::Init_ConfigureCVars();
  m_bSettingsWritable = !xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-no-file-cvars", false);
  m_Settings.m_bVSync = cvar_AppVSync.GetValue();
  if (const auto* limit = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit")))
    m_Settings.m_uiFrameLimit = xiiMath::Clamp(limit->GetValue(), 0, 1000);
  if (m_Settings.Load(":project/RuntimeConfigs/Presentation.ddl", m_Settings.m_bVSync).Failed())
    xiiLog::Warning("Cannot read the project's presentation defaults; using engine defaults.");
  if (m_bSettingsWritable && m_Settings.Load(":appdata/RuntimeConfigs/Presentation.ddl", m_Settings.m_bVSync).Failed())
  {
    m_bSettingsWritable = false;
    xiiLog::Warning("Cannot read user presentation settings; preserving the original file and using project defaults.");
  }
  const auto* command = xiiCommandLineUtils::GetGlobalInstance();
  if (command->HasOption("-App.VSync"))
    m_Settings.m_bVSync = command->GetBoolOption("-App.VSync", m_Settings.m_bVSync);
  if (command->HasOption("-App.FrameLimit"))
    m_Settings.m_uiFrameLimit = xiiMath::Clamp(command->GetIntOption("-App.FrameLimit", m_Settings.m_uiFrameLimit), 0, 1000);
  if (command->HasOption("-background-fps"))
    m_Settings.m_uiBackgroundFrameLimit = xiiMath::Clamp(command->GetIntOption("-background-fps", 30), 1, 1000);
  if (command->HasOption("-show-fps"))
    m_Settings.m_bShowFrameStats = command->GetBoolOption("-show-fps", false);
  if (command->HasOption("-volume"))
    m_Settings.m_fMasterVolume = xiiMath::Clamp(static_cast<float>(command->GetFloatOption("-volume", 1.0)), 0.0f, 1.0f);
  // Startup overrides are ephemeral. Save only explicit runtime commits, never startup resolution.
  m_Settings.NotifyChanged();
  m_bSettingsInitialized = true;
}

xiiResult xiiGameApplication::ApplySettings(const xiiGameApplicationSettings& settings)
{
  XII_SUCCEED_OR_RETURN(m_Settings.Apply(settings));
  // Explicit commits also retry an unchanged value after an earlier write failure.
  if (m_bSettingsInitialized && m_bSettingsWritable)
    return m_Settings.Save(":appdata/RuntimeConfigs/Presentation.ddl");
  return XII_SUCCESS;
}

void xiiGameApplication::OnSettingsChanged(xiiSettings*)
{
  cvar_AppVSync = m_Settings.m_bVSync;
  if (auto* limit = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit")))
    *limit = static_cast<xiiInt32>(m_Settings.m_uiFrameLimit);
  if (m_bSettingsInitialized && m_bSettingsWritable && m_Settings.Save(":appdata/RuntimeConfigs/Presentation.ddl").Failed())
    xiiLog::Error("Failed to save runtime presentation settings.");
  auto pWindowManager = xiiWindowManager::GetSingleton();
  if (pWindowManager == nullptr)
    return;

  xiiTemporaryHybridArray<xiiRegisteredWindowHandle, 8> windows;
  pWindowManager->GetRegistered(windows);

  for (xiiRegisteredWindowHandle hWindow : windows)
  {
    xiiWindowBase* pWindow = pWindowManager->GetWindow(hWindow);
    if (pWindow == nullptr)
      continue;

    if (xiiWindowOutputTargetBase* pOutput = pWindow->GetOutputTarget())
    {
      pOutput->SetVSyncEnabled(m_Settings.m_bVSync);
    }
  }
  ApplyViewSettings();
}

namespace
{
  const char* s_szInputSet               = "GameApp";
  const char* s_szCloseAppAction         = "CloseApp";
  const char* s_szShowConsole            = "ShowConsole";
  const char* s_szShowFpsAction          = "ShowFps";
  const char* s_szReloadResourcesAction  = "ReloadResources";
  const char* s_szCaptureProfilingAction = "CaptureProfiling";
  const char* s_szCaptureFrame           = "CaptureFrame";
  const char* s_szTakeScreenshot         = "TakeScreenshot";
} // namespace

void xiiGameApplication::Init_ConfigureInput()
{
  xiiInputActionConfig config;

  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szCloseAppAction, config, true);

  // the tilde has problematic behavior on keyboards where it is a hat (^)
  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF1;
  xiiInputManager::SetInputActionConfig("Console", s_szShowConsole, config, true);

  // in the editor we cannot use F5, because that is already 'run application'
  // so we use F4 there, and it should be consistent here
  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF4;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szReloadResourcesAction, config, true);

  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF5;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);

  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF8;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);

  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF11;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);

  config.m_sInputSlotTrigger[0] = xiiInputSlot_KeyF12;
  xiiInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);

  {
    xiiStringView sConfigFile = xiiGameAppInputConfig::s_sConfigFile;

    xiiFileReader file;
    if (file.Open(sConfigFile).Succeeded())
    {
      xiiHybridArray<xiiGameAppInputConfig, 32> InputActions;

      xiiGameAppInputConfig::ReadFromDDL(file, InputActions);
      xiiGameAppInputConfig::ApplyAll(InputActions);
    }
  }

  if (m_pConsole)
  {
    m_pConsole->LoadInputHistory(":appdata/ConsoleInputHistory.cfg");
  }
}

bool xiiGameApplication::Run_ProcessApplicationInput()
{
  // the show console command must be in the "Console" input set, because we are using that for exclusive input when the console is open
  if (xiiInputManager::GetInputActionState("Console", s_szShowConsole) == xiiKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;

    if (m_bShowConsole)
      xiiInputManager::SetExclusiveInputSet("Console");
    else
    {
      xiiInputManager::SetExclusiveInputSet("");
      m_pConsole->SaveInputHistory(":appdata/ConsoleInputHistory.cfg").IgnoreResult();
    }
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szShowFpsAction) == xiiKeyState::Pressed)
  {
    if (auto* state = xiiDynamicCast<xiiGameState*>(m_pGameState.Borrow()))
      if (auto* world = state->GetMainWorld())
      {
        XII_LOCK(world->GetWriteMarker());
        if (auto* view = state->GetMainView())
        {
          const bool show = !view->GetShowFrameStats();
          view->SetShowFrameStats(show);
          if (auto* cameras = world->GetComponentManager<xiiCameraComponentManager>())
            for (auto camera = cameras->GetComponents(); camera.IsValid(); ++camera)
              if (camera->IsActive() && camera->GetUsageHint() == xiiCameraUsageHint::MainView)
              {
                camera->SetShowFrameStats(show);
                break;
              }
        }
      }
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szReloadResourcesAction) == xiiKeyState::Pressed)
  {
    xiiResourceManager::ReloadAllResources(false);
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szTakeScreenshot) == xiiKeyState::Pressed)
  {
    TakeScreenshot();
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szCaptureProfilingAction) == xiiKeyState::Pressed)
  {
    TakeProfilingCapture();
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szCaptureFrame) == xiiKeyState::Pressed)
  {
    CaptureFrame();
  }

  if (m_pConsole)
  {
    m_pConsole->DoDefaultInputHandling(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (xiiInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == xiiKeyState::Pressed)
  {
    RequestQuit();
  }

  return true;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);

void xiiGameApplication::InitializeManagedSession()
{
  if (m_pSessionChannel)
    return;
  auto* command = xiiCommandLineUtils::GetGlobalInstance();
  m_sSession    = command->GetStringOption("-player-session");
  m_sInstance   = command->GetStringOption("-player-instance");
  if (m_sSession.IsEmpty())
    return;
  auto validIdentifier = [](xiiStringView value) {
    if (value.IsEmpty() || value.GetElementCount() > 100)
      return false;
    for (const auto character : value)
      if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') || character == '-'))
        return false;
    return true;
  };
  if (!validIdentifier(m_sSession) || !validIdentifier(m_sInstance) || command->GetStringOption("-player-ipc") != m_sInstance)
  {
    xiiLog::Error("Invalid managed Player identity or private IPC endpoint.");
    SetReturnCode(2);
    RequestQuit();
    return;
  }
  const xiiString project = command->GetStringOption("-project");
  if (!project.IsAbsolutePath())
  {
    xiiLog::Error("Managed application project paths must be absolute.");
    SetReturnCode(2);
    RequestQuit();
    return;
  }
  m_sAppProjectPath = project;
  m_pSessionChannel = xiiIpcChannel::CreatePipeChannel(m_sInstance, xiiIpcChannel::Mode::Client);
  if (!m_pSessionChannel)
  {
    SetReturnCode(2);
    RequestQuit();
    return;
  }
  m_pSessionProtocol = XII_DEFAULT_NEW(xiiIpcProcessMessageProtocol, m_pSessionChannel.Borrow());
  m_pSessionProtocol->m_MessageEvent.AddEventHandler([this](const xiiIpcProcessMessageProtocol::Event& event) {
    if (const auto* scene = xiiDynamicCast<const xiiPlayerSceneMessage*>(event.m_pMessage))
    {
      if (auto* state = xiiGameState::GetActiveGameState())
      {
        if (scene->m_bCancel)
          state->CancelBackgroundSceneLoading();
        else if (state->TransitionToScene(scene->m_sScene, scene->m_sPreloadCollection).Failed())
          ReportStatus("failed", "Invalid scene transition request; provide a nonempty scene reference.");
      }
    }
    if (event.m_pMessage->IsInstanceOf<xiiPlayerStopMessage>())
    {
      ReportStatus("stopping", "Graceful stop requested by editor.");
      RequestQuit();
    }
  });
  m_pSessionChannel->Connect();
}

void xiiGameApplication::AfterCoreSystemsStartup()
{
  InitializeManagedSession();
  if (!WasQuitRequested())
    SUPER::AfterCoreSystemsStartup();
}

void xiiGameApplication::Init_FileSystem_ConfigureDataDirs()
{
  SUPER::Init_FileSystem_ConfigureDataDirs();
  if (m_sSession.IsEmpty() || WasQuitRequested())
    return;
  xiiStringBuilder projectKey;
  projectKey.SetFormat("{}", xiiArgU(xiiHashingUtils::StringHash(FindProjectDirectory()), 16, true, 16));
  xiiStringBuilder directory(xiiOSFile::GetUserDataFolder(), "/", GetApplicationName(), "/", projectKey);
  directory.AppendPath("Sessions", m_sInstance);
  if (xiiOSFile::CreateDirectoryStructure(directory).Failed() ||
      m_ManagedPreferencesLock.Open(xiiStringBuilder(directory, "/Preferences.lock"), xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Failed())
  {
    ReportStatus("failed", "Cannot create or exclusively lock managed application preferences.");
    SetReturnCode(4);
    RequestQuit();
    return;
  }
  m_sManagedWritableDirectory = directory;
  // Shared permutations are read-only and have lower lookup priority than private outputs.
  xiiFileSystem::AddDataDirectory(">sdk/Output/", "ManagedPlayer", "", xiiDataDirUsage::ReadOnly).IgnoreResult();
  for (const char* root : {"appdata", "bin", "shadercache"})
  {
    xiiFileSystem::RemoveDataDirectory(root);
    if (xiiFileSystem::AddDataDirectory(directory, "ManagedPlayer", root, xiiDataDirUsage::AllowWrites).Failed())
    {
      ReportStatus("failed", "Cannot mount private managed application data.");
      SetReturnCode(4);
      RequestQuit();
      return;
    }
  }
  const xiiString temp = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-player-temp");
  if (temp.IsEmpty() || !temp.IsAbsolutePath())
  {
    ReportStatus("failed", "Managed application temporary directory must be absolute.");
    SetReturnCode(4);
    RequestQuit();
    return;
  }
  xiiFileSystem::SetSpecialDirectory("temp", temp);
  xiiLog::Info("Managed application '{}' uses private data '{}'.", m_sInstance, directory);
}

void xiiGameApplication::Init_ConfigureTelemetry()
{
  SUPER::Init_ConfigureTelemetry();
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && defined(BUILDSYSTEM_ENABLE_ENET_SUPPORT)
  if (!m_sSession.IsEmpty() && xiiTelemetry::GetConnectionMode() != xiiTelemetry::Server)
  {
    ReportStatus("failed", "Cannot bind the assigned telemetry port. Select a different launch base port.");
    SetReturnCode(6);
    RequestQuit();
  }
#endif
}

xiiApplication::Execution xiiGameApplication::Run()
{
  PumpSession();
  return SUPER::Run();
}

void xiiGameApplication::PumpSession()
{
  if (!m_pSessionChannel)
    return;
  if (m_pSessionChannel->GetConnectionState() == xiiIpcChannel::ConnectionState::Disconnected &&
      xiiTime::Now() - m_LastConnectAttempt > xiiTime::MakeFromMilliseconds(200))
  {
    if (m_bSessionConnected)
    {
      RequestQuit();
      return;
    }
    m_LastConnectAttempt = xiiTime::Now();
    m_pSessionChannel->Connect();
  }
  m_pSessionProtocol->ProcessMessages();
  if (m_pSessionChannel->IsConnected())
  {
    m_bSessionConnected = true;
    if (xiiTime::Now() - m_LastStatus > xiiTime::MakeFromSeconds(1))
      ReportStatus(m_sStage, m_sDetail);
  }
}

void xiiGameApplication::ReportStatus(xiiStringView sStage, xiiStringView sDetail, float fProgress)
{
  if (m_sStage == sStage && m_sDetail == sDetail && xiiTime::Now() - m_LastStatus < xiiTime::MakeFromMilliseconds(500))
    return;
  if (m_sStage != sStage)
    m_sStage = sStage;
  if (m_sDetail != sDetail)
    m_sDetail = sDetail;
  m_LastStatus = xiiTime::Now();
  if (m_pSessionChannel && m_pSessionChannel->IsConnected())
  {
    xiiPlayerStatusMessage status;
    status.m_sInstance = m_sInstance;
    status.m_sStage    = m_sStage;
    status.m_sDetail   = m_sDetail;
    status.m_fProgress = fProgress;
    if (const auto* state = xiiGameState::GetActiveGameState())
      status.m_uiSceneRevision = state->GetWorldRevision();
    m_pSessionProtocol->Send(&status);
  }
}

void xiiGameApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();
  // The editor owns play-mode teardown and must keep its engine process alive.
  if (!xiiStartup::HasApplicationTag("editorengineprocess"))
    if (auto* active = GetActiveGameState())
      if (active->WasQuitRequested())
        RequestQuit();
  if (auto* state = xiiGameState::GetActiveGameState())
  {
    if (state->GetWorldRevision() != m_uiConfiguredWorldRevision)
    {
      m_uiConfiguredWorldRevision = state->GetWorldRevision();
      ApplyViewSettings();
    }
    if (!state->GetSceneLoadingError().IsEmpty())
      ReportStatus("failed", state->GetSceneLoadingError());
    else if (state->IsLoadingSceneInBackground())
    {
      const bool activating = state->GetSceneLoadingPhase() == xiiSceneLoadUtility::LoadingPhase::Instantiating;
      float      progress   = 0.0f;
      state->IsLoadingSceneInBackground(&progress);
      ReportStatus(activating ? "activating" : "loading", activating ? "Activating incoming scene" : "Reading scene and preloading resources", state->HasMeasuredSceneLoadingProgress() ? progress : -1.0f);
    }
    else
    {
      xiiStringBuilder ready;
      ready.SetFormat("Ready: {}", state->GetActiveScene());
      ReportStatus("running", ready);
    }
  }
  else if (GetActiveGameState())
    ReportStatus("running", "Application game state active");
  if (auto* sound = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>(); sound && !xiiStartup::HasApplicationTag("editorengineprocess"))
  {
    sound->SetMasterChannelVolume(GetSettings().m_fMasterVolume);
    auto*       state  = xiiGameState::GetActiveGameState();
    const auto* window = state ? state->GetMainWindow() : nullptr;
    sound->SetMasterChannelMute(GetSettings().m_bMuteInBackground && window && !window->HasFocus());
  }
}

void xiiGameApplication::BeforeHighLevelSystemsShutdown()
{
  ReportStatus("stopping", "Releasing scene and application services");
  SUPER::BeforeHighLevelSystemsShutdown();
}

void xiiGameApplication::BeforeCoreSystemsShutdown()
{
  SUPER::BeforeCoreSystemsShutdown();
  ReportStatus("exited", "Shutdown complete");
  m_pSessionProtocol.Clear();
  m_pSessionChannel.Clear();
  m_ManagedPreferencesLock.Close();
}

xiiUInt32 xiiGameApplication::GetFrameLimit() const
{
  const auto  foregroundLimit = SUPER::GetFrameLimit();
  auto*       state           = xiiGameState::GetActiveGameState();
  const auto* window          = state ? state->GetMainWindow() : nullptr;
  if (window && (!window->HasFocus() || !window->IsVisible()))
    return foregroundLimit == 0 ? GetSettings().m_uiBackgroundFrameLimit : xiiMath::Min(foregroundLimit, GetSettings().m_uiBackgroundFrameLimit);
  return foregroundLimit;
}

void xiiGameApplication::ApplyViewSettings()
{
  // Editor view/camera preferences are owned and synchronized by the editor's viewport controls.
  if (xiiStartup::HasApplicationTag("editorengineprocess"))
    return;
  auto* state = xiiGameState::GetActiveGameState();
  if (!state || !state->GetMainWorld())
    return;
  XII_LOCK(state->GetMainWorld()->GetWriteMarker());
  if (auto* view = state->GetMainView())
  {
    view->SetShowFrameStats(GetSettings().m_bShowFrameStats);
  }
  if (auto* cameras = state->GetMainWorld()->GetComponentManager<xiiCameraComponentManager>())
    for (auto camera = cameras->GetComponents(); camera.IsValid(); ++camera)
      if (camera->IsActive() && camera->GetUsageHint() == xiiCameraUsageHint::MainView)
      {
        camera->SetShowFrameStats(GetSettings().m_bShowFrameStats);
      }
}
