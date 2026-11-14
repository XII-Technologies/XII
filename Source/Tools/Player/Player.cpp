/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <GameEngine/GameApplication/PlayerMessages.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/View.h>
#include <Player/Player.h>

XII_APPLICATION_ENTRY_POINT(xiiPlayerApplication);

xiiCommandLineOptionString opt_Project("_Player", "-project", "Project directory. Relative paths resolve against the executable, then the SDK.", "");
xiiCommandLineOptionString opt_Scene("_Player", "-scene", "Startup scene asset GUID or data-directory path. Overrides the project main scene.", "");
xiiCommandLineOptionString opt_Preload("_Player", "-preload", "Optional collection asset GUID or data-directory path to preload before scene activation.", "");
xiiCommandLineOptionString opt_WindowTitle("_Player", "-window-title", "Optional main window title override.", "");
xiiCommandLineOptionInt    opt_WindowX("_Player", "-window-x", "Main window position override in desktop coordinates.", 0, -32768, 32767);
xiiCommandLineOptionInt    opt_WindowY("_Player", "-window-y", "Main window position override in desktop coordinates.", 0, -32768, 32767);
xiiCommandLineOptionString opt_Launch("_Player", "-launch", "Named configuration in RuntimeConfigs/Player.ddl, applied to this instance.", "");
xiiCommandLineOptionString opt_LaunchFile("_Player", "-launch-file", "Optional absolute path to a Player.ddl launch configuration.", "");
xiiCommandLineOptionString opt_WindowMode("_Player", "-window-mode", "window, borderless or fullscreen.", "");
xiiCommandLineOptionInt    opt_Width("_Player", "-width", "Window width in pixels.", 1280, 320, 16384);
xiiCommandLineOptionInt    opt_Height("_Player", "-height", "Window height in pixels.", 720, 200, 16384);
xiiCommandLineOptionInt    opt_Display("_Player", "-display", "Display index (-1 selects the primary display).", -1, -1, 127);
xiiCommandLineOptionInt    opt_BackgroundFPS("_Player", "-background-fps", "Inactive instance frame cap.", 30, 1, 1000);
xiiCommandLineOptionBool   opt_ShowFPS("_Player", "-show-fps", "Main-view FPS and frame-time overlay.", false);
xiiCommandLineOptionFloat  opt_Volume("_Player", "-volume", "Master volume when a sound implementation is present.", 1.0f, 0.0f, 1.0f);
xiiCommandLineOptionString opt_Role("_Player", "-role", "Application role: standalone, server or client. Consumed by application networking plugins.", "standalone");
xiiCommandLineOptionString opt_Connect("_Player", "-connect", "Application connection address; no gameplay networking stack is created by Player.", "127.0.0.1");
xiiCommandLineOptionInt    opt_Port("_Player", "-port", "Application connection port; supplied to application networking plugins.", 0, 0, 65535);

xiiPlayerApplication::xiiPlayerApplication() : xiiGameApplication("xiiPlayer", nullptr) {}

xiiResult xiiPlayerApplication::BeforeCoreSystemsStartup()
{
  xiiStringBuilder help;
  if (xiiCommandLineOption::LogAvailableOptionsToBuffer(help, xiiCommandLineOption::LogAvailableModes::IfHelpRequested))
  {
    xiiLog::OsMessageBox(help);
    return XII_FAILURE;
  }
  xiiStartup::AddApplicationTag("player");
  XII_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());
  DetermineProjectPath();
  return XII_SUCCESS;
}

void xiiPlayerApplication::AfterCoreSystemsStartup()
{
  auto* command = xiiCommandLineUtils::GetGlobalInstance();
  InitializeManagedSession();
  if (WasQuitRequested())
  {
    return;
  }
  if (m_sSession.IsEmpty() && m_sInstance.IsEmpty())
  {
    xiiStringBuilder identifier;
    xiiUInt64        low, high;
    xiiUuid::MakeUuid().GetValues(low, high);
    identifier.SetFormat("standalone-{}{}", xiiArgU(high, 16, true, 16), xiiArgU(low, 16, true, 16));
    m_sInstance = identifier;
    command->InjectCustomArgument("-player-instance");
    command->InjectCustomArgument(m_sInstance);
  }
  if (xiiFileSystem::AddDataDirectory("", "PlayerBootstrap", ":", xiiDataDirUsage::ReadOnly).Failed() || ResolveLaunchConfiguration().Failed())
  {
    ReportStatus("failed", "Invalid launch configuration or runtime arguments. Check Player.ddl, project and command-line values.");
    SetReturnCode(2);
    RequestQuit();
    return;
  }
  m_LogSubscription = xiiGlobalLog::AddLogWriter(xiiMakeDelegate(&xiiPlayerApplication::LogMessage, this));
  m_pStartupWindow  = XII_DEFAULT_NEW(xiiWindow);
  xiiWindowCreationDescription window;
  window.m_Title                = "XII Player - Initializing";
  window.m_WindowMode           = xiiWindowMode::WindowResizable;
  window.m_bClipMouseCursor     = false;
  window.m_bShowMouseCursor     = true;
  window.m_bSetForegroundOnInit = command->GetUIntOption("-player-index", 0) == 0;
  if (m_pStartupWindow->Initialize(window).Failed())
  {
    ReportStatus("failed", "Cannot create a startup window. Check the display and SDL diagnostics.");
    SetReturnCode(3);
    RequestQuit();
    m_pStartupWindow.Clear();
    return;
  }
  m_pStartupWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& e) {
    if (e.m_Type == xiiWindowEvent::CloseButtonClicked)
    {
      RequestQuit();
    }
  });
}

xiiApplication::Execution xiiPlayerApplication::Run()
{
  PumpSession();
  if (WasQuitRequested())
  {
    return Execution::Quit;
  }
  if (!m_bInitializing)
  {
    return SUPER::Run();
  }
  if (m_pStartupWindow)
  {
    m_pStartupWindow->ProcessWindowMessages();
  }
  if (WasQuitRequested())
  {
    return Execution::Quit;
  }

  // Thread-affine startup stages remain on the main thread, with OS/IPC pumping between stages.
  // Scene IO/description work is asynchronous; plugin initialization and GPU creation are not worker-safe.
  switch (m_uiInitStep++)
  {
    case 0:
      ReportStatus("initializing", "Selecting platform and project");
      Init_PlatformProfile_SetPreferred();
      Init_FileSystem_SetSpecialDirs();
      break;
    case 1:
      ReportStatus("initializing", "Loading engine plugins");
      Init_LoadRequiredPlugins();
      Init_ConfigureAssetManagement();
      break;
    case 2:
      ReportStatus("initializing", "Mounting project data");
      Init_FileSystem_ConfigureDataDirs();
      break;
    case 3:
      ReportStatus("initializing", "Loading project plugins");
      Init_LoadWorldModuleConfig();
      Init_LoadProjectPlugins();
      break;
    case 4:
      ReportStatus("initializing", "Loading runtime configuration");
      Init_PlatformProfile_LoadForRuntime();
      Init_ConfigureInput();
      Init_ConfigureTags();
      Init_ConfigureCVars();
      Init_ConfigureTelemetry();
      break;
    case 5:
      ReportStatus("initializing", "Creating graphics device");
      Init_SetupGraphicsDevice();
      break;
    case 6:
      ReportStatus("initializing", "Preparing default resources");
      Init_SetupDefaultResources();
      break;
    default:
      if (!WasQuitRequested())
      {
        xiiStartup::StartupHighLevelSystems();
        ActivateGameStateAtStartup();
        // Custom game-state implementations may create their own windows instead of taking this one.
        m_pStartupWindow.Clear();
        m_bInitializing = false;
      }
      break;
  }
  xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  return WasQuitRequested() ? Execution::Quit : Execution::Continue;
}

void xiiPlayerApplication::Init_FileSystem_ConfigureDataDirs()
{
  xiiFileSystem::RemoveDataDirectoryGroup("PlayerBootstrap");
  SUPER::Init_FileSystem_ConfigureDataDirs();
  if (WasQuitRequested())
  {
    return;
  }
  if (!m_sSession.IsEmpty())
  {
    m_sWritableDirectory = m_sManagedWritableDirectory;
    m_Log.BeginLog(":appdata/Player.html", "XII Player");
    xiiLog::Info("Player instance '{}', project '{}', writable data '{}'.", m_sInstance, m_sAppProjectPath, m_sWritableDirectory);
    return;
  }
  // Standalone preferences are per project. Only one process may write them. Further standalone
  // instances and all editor instances get private roots, including screenshots, CVars and shader cache.
  xiiStringBuilder projectKey;
  projectKey.SetFormat("{}", xiiArgU(xiiHashingUtils::StringHash(m_sAppProjectPath), 16, true, 16));
  xiiStringBuilder directory(xiiOSFile::GetUserDataFolder(), "/xiiPlayer/", projectKey);
  if (!m_sSession.IsEmpty())
  {
    directory.AppendPath("Sessions", m_sInstance);
  }
  if (xiiOSFile::CreateDirectoryStructure(directory).Failed())
  {
    ReportStatus("failed", "Cannot create the Player writable directory.");
    SetReturnCode(4);
    RequestQuit();
    return;
  }
  if (m_UserSettingsLock.Open(xiiStringBuilder(directory, "/Preferences.lock"), xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Failed())
  {
    directory.AppendPath("Instances");
    directory.AppendPath(m_sInstance);
    if (xiiOSFile::CreateDirectoryStructure(directory).Failed() ||
        m_UserSettingsLock.Open(xiiStringBuilder(directory, "/Preferences.lock"), xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Failed())
    {
      ReportStatus("failed", "Cannot lock private Player preferences.");
      SetReturnCode(4);
      RequestQuit();
      return;
    }
  }
  m_sWritableDirectory = directory;
  // Shared permutations are read-only and have lower lookup priority than private outputs.
  xiiFileSystem::AddDataDirectory(">sdk/Output/", "Player", "", xiiDataDirUsage::ReadOnly).IgnoreResult();
  for (const char* root : {"appdata", "bin", "shadercache"})
  {
    xiiFileSystem::RemoveDataDirectory(root);
    if (xiiFileSystem::AddDataDirectory(directory, "Player", root, xiiDataDirUsage::AllowWrites).Failed())
    {
      ReportStatus("failed", "Cannot mount private Player writable data.");
      SetReturnCode(4);
      RequestQuit();
      return;
    }
  }
  xiiStringBuilder temp = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-player-temp");
  if (temp.IsEmpty())
  {
    temp.SetFormat("{}/xiiPlayer/{}", xiiOSFile::GetTempDataFolder(), m_sInstance);
    m_sOwnedTemporaryDirectory = temp;
  }
  if (!temp.IsAbsolutePath() || xiiOSFile::CreateDirectoryStructure(temp).Failed())
  {
    ReportStatus("failed", "Cannot create the private temporary directory.");
    SetReturnCode(4);
    RequestQuit();
    return;
  }
  xiiFileSystem::SetSpecialDirectory("temp", temp);
  m_Log.BeginLog(":appdata/Player.html", "XII Player");
  xiiLog::Info("Player instance '{}', project '{}', writable data '{}'.", m_sInstance, m_sAppProjectPath, directory);
}

void xiiPlayerApplication::Init_ConfigureTelemetry()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && defined(BUILDSYSTEM_ENABLE_ENET_SUPPORT)
  const auto*            command      = xiiCommandLineUtils::GetGlobalInstance();
  const bool             explicitPort = command->HasOption("-TelemetryPort");
  xiiUInt32              port         = command->GetUIntOption("-TelemetryPort", 1040);
  const xiiStringBuilder directory(xiiOSFile::GetTempDataFolder(), "/xiiPlayer-ports");
  if (m_sSession.IsEmpty() && xiiOSFile::CreateDirectoryStructure(directory).Failed())
  {
    ReportStatus("failed", "Cannot create the telemetry port lease directory.");
    SetReturnCode(6);
    RequestQuit();
    return;
  }
  for (xiiUInt32 attempt = 0; attempt < (explicitPort ? 1U : 64U); ++attempt)
  {
    if (attempt > 0)
    {
      port = 1041 + (xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(m_sInstance)) + attempt) % 18000;
    }
    if (m_sSession.IsEmpty())
    {
      m_TelemetryPortLease.Close();
      xiiStringBuilder path;
      path.SetFormat("{}/{}.lock", directory, port);
      if (m_TelemetryPortLease.Open(path, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Failed())
      {
        continue;
      }
    }
    xiiTelemetry::s_uiPort = static_cast<xiiUInt16>(port);
    xiiTelemetry::SetServerName(m_sInstance);
    xiiTelemetry::CreateServer();
    if (xiiTelemetry::GetConnectionMode() == xiiTelemetry::Server)
    {
      xiiLog::Info("Player '{}' telemetry listens on UDP port {}.", m_sInstance, port);
      return;
    }
  }
  ReportStatus("failed", "Cannot bind the telemetry port. Stop the conflicting application or choose -TelemetryPort <free port>.");
  SetReturnCode(6);
  RequestQuit();
#endif
}

void xiiPlayerApplication::Init_ConfigureCVars()
{
  // Keep the persisted layer separate from launch overrides. Startup never rewrites preferences.
  m_PersistedSettings.Load(":project/RuntimeConfigs/Presentation.ddl", true).IgnoreResult();
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-no-file-cvars", false))
  {
    m_PersistedSettings.Load(":appdata/RuntimeConfigs/Presentation.ddl", m_PersistedSettings.m_bVSync).IgnoreResult();
  }
  SUPER::Init_ConfigureCVars();
}

xiiResult xiiPlayerApplication::ResolveLaunchConfiguration()
{
  auto*           command = xiiCommandLineUtils::GetGlobalInstance();
  const xiiString name    = command->GetStringOption("-launch");
  if (!name.IsEmpty())
  {
    xiiPlayerConfig  config;
    xiiStringBuilder path = command->GetStringOption("-launch-file");
    if (path.IsEmpty())
    {
      path.Set(m_sAppProjectPath, "/RuntimeConfigs/Player.ddl");
    }
    if (!path.IsAbsolutePath())
    {
      path.Prepend(m_sAppProjectPath, "/");
    }
    path.MakeCleanPath();
    XII_SUCCEED_OR_RETURN(config.Load(path));
    const auto* launch = config.FindLaunch(name);
    if (!launch)
    {
      xiiLog::Error("Named Player configuration '{}' was not found in '{}'.", name, path);
      return XII_FAILURE;
    }
    if (!launch->m_sProject.IsEmpty())
    {
      xiiStringBuilder project = launch->m_sProject;
      if (project.IsRelativePath())
      {
        project.Prepend(m_sAppProjectPath, "/");
      }
      project.MakeCleanPath();
      m_sAppProjectPath = project;
    }
    xiiDynamicArray<xiiString> effective = command->GetCommandLineArray();
    const auto                 index     = command->GetUIntOption("-player-index", 0);
    if (index < launch->m_InstanceArguments.GetCount())
    {
      xiiDynamicArray<xiiString>   instance;
      xiiDynamicArray<const char*> argv;
      xiiCommandLineUtils::SplitCommandLineString(launch->m_InstanceArguments[index], false, instance, argv);
      for (const auto& arg : instance)
      {
        effective.PushBack(arg);
      }
    }
    for (const auto& arg : launch->m_Arguments)
    {
      effective.PushBack(arg);
    }
    if (!launch->m_sScene.IsEmpty())
    {
      effective.PushBack("-scene");
      effective.PushBack(launch->m_sScene);
    }
    effective.PushBack("-role");
    effective.PushBack(launch->m_sRole == "server-clients" ? (index == 0 ? "server" : "client") : launch->m_sRole.GetData());
    effective.PushBack("-connect");
    effective.PushBack(launch->m_sConnect);
    if (launch->m_uiBasePort != 0)
    {
      effective.PushBack("-port");
      xiiStringBuilder port;
      port.SetFormat("{}", launch->m_uiBasePort + (launch->m_sRole == "server-clients" ? 0 : index));
      effective.PushBack(port);
    }
    command->SetCommandLine(effective);
  }
  const auto validation = xiiPlayerLaunchConfig::ValidateRuntimeArguments(command->GetCommandLineArray());
  if (validation.Failed())
  {
    xiiLog::Error("Invalid Player launch: {}", validation.GetMessageString());
    return XII_FAILURE;
  }
  const auto mode = command->GetStringOption("-window-mode");
  if (!mode.IsEmpty() && mode != "window" && mode != "borderless" && mode != "fullscreen")
  {
    return XII_FAILURE;
  }
  if (command->GetIntOption("-width", 1280) < 320 || command->GetIntOption("-width", 1280) > 16384 ||
      command->GetIntOption("-height", 720) < 200 || command->GetIntOption("-height", 720) > 16384)
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiUniquePtr<xiiWindow> xiiPlayerApplication::TakeStartupWindow()
{
  // The temporary startup callback is application-owned and remains valid after this transfer.
  return std::move(m_pStartupWindow);
}

void xiiPlayerApplication::ReportStatus(xiiStringView sStage, xiiStringView sDetail, float fProgress)
{
  if (m_pStartupWindow && m_sDetail != sDetail)
  {
    xiiStringBuilder title;
    title.SetFormat("XII Player - {}", sDetail);
    m_pStartupWindow->SetTitle(title).IgnoreResult();
  }
  SUPER::ReportStatus(sStage, sDetail, fProgress);
}

void xiiPlayerApplication::LogMessage(const xiiLoggingEventData& e)
{
  XII_LOCK(m_LogMutex);
  m_Log.LogMessageHandler(e);
}

xiiWindow* xiiPlayerApplication::GetPlayerWindow() const
{
  if (!xiiGameState::GetActiveGameState())
  {
    return nullptr;
  }
  xiiHybridArray<xiiRegisteredWindowHandle, 4> windows;
  xiiWindowManager::GetSingleton()->GetRegistered(windows, GetActiveGameState());
  return windows.IsEmpty() ? nullptr : static_cast<xiiWindow*>(xiiWindowManager::GetSingleton()->GetWindow(windows[0]));
}

void xiiPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();
  UpdateRuntimeSettings();
}

xiiResult xiiPlayerApplication::ApplyDisplayPreview()
{
  auto* window = GetPlayerWindow();
  if (!window || m_PendingSettings.Validate().Failed())
  {
    return XII_FAILURE;
  }
  m_PreviousWindow          = window->GetCreationDescription();
  const bool displayChanged = m_PreviousWindow.m_WindowMode != m_PendingWindow.m_WindowMode ||
    m_PreviousWindow.m_iMonitor != m_PendingWindow.m_iMonitor || m_PreviousWindow.m_Resolution != m_PendingWindow.m_Resolution;
  if (displayChanged)
  {
    if (window->ApplyWindowSettings(m_PendingWindow).Failed())
    {
      window->ApplyWindowSettings(m_PreviousWindow).IgnoreResult();
      return XII_FAILURE;
    }
    m_bDisplayPreview        = true;
    m_DisplayPreviewDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(15);
    m_sSettingsMessage       = "Keep display changes? Enter confirms. Escape reverts.";
  }
  else
  {
    XII_SUCCEED_OR_RETURN(ApplySettings(m_PendingSettings));
    ApplyViewSettings();
    m_bRuntimeSettingsCommitted = true;
    m_sSettingsMessage          = "Settings applied.";
  }
  return XII_SUCCESS;
}

void xiiPlayerApplication::RevertDisplayPreview()
{
  if (auto* window = GetPlayerWindow())
  {
    if (window->ApplyWindowSettings(m_PreviousWindow).Failed())
    {
      // A monitor may have disappeared during preview. Recover to a window on the primary display.
      m_PreviousWindow.m_WindowMode = xiiWindowMode::WindowResizable;
      m_PreviousWindow.m_iMonitor   = 0;
      m_PreviousWindow.m_Resolution = xiiSizeU32(1280, 720);
      if (window->ApplyWindowSettings(m_PreviousWindow).Failed())
      {
        xiiLog::Error("Cannot restore the display. Restart Player with -window-mode window -display 0.");
      }
    }
  }
  m_PendingWindow    = m_PreviousWindow;
  m_bDisplayPreview  = false;
  m_sSettingsMessage = "Display changes reverted; saved preferences were preserved.";
}

void xiiPlayerApplication::UpdateRuntimeSettings()
{
  auto* window = GetPlayerWindow();
  auto* state  = xiiGameState::GetActiveGameState();
  if (!window || !state || !state->GetMainWorld())
  {
    return;
  }
  auto pressed = [](const char* slot) {
    return xiiInputManager::GetInputSlotState(slot) == xiiKeyState::Pressed;
  };
  if (m_bDisplayPreview && (xiiTime::Now() >= m_DisplayPreviewDeadline || !window->HasFocus()))
  {
    RevertDisplayPreview();
  }
  if (pressed(xiiInputSlot_KeyF2))
  {
    if (m_bDisplayPreview)
    {
      RevertDisplayPreview();
    }
    m_bShowSettings = !m_bShowSettings;
    xiiInputManager::SetExclusiveInputSet(m_bShowSettings ? "PlayerSettings" : "");
    if (m_bShowSettings)
    {
      m_bShowConsole = false;
      m_PendingSettings.Apply(GetSettings()).IgnoreResult();
      if (const auto* cap = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit")))
      {
        m_PendingSettings.m_uiFrameLimit = xiiMath::Clamp(cap->GetValue(), 0, 1000);
      }
      m_PendingWindow = window->GetCreationDescription();
      m_sSettingsMessage.Clear();
    }
  }
  if (m_bShowSettings && pressed(xiiInputSlot_KeyF1))
  {
    if (m_bDisplayPreview)
    {
      RevertDisplayPreview();
    }
    m_bShowSettings = false;
    m_bShowConsole  = true;
    xiiInputManager::SetExclusiveInputSet("Console");
  }
  if (!m_bShowSettings)
  {
    if (auto* input = window->GetInputDevice())
    {
      const auto& creation = window->GetCreationDescription();
      input->SetClipMouseCursor(window->HasFocus() && !m_bShowConsole && creation.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindow : xiiMouseCursorClipMode::NoClip);
      input->SetShowMouseCursor(!window->HasFocus() || m_bShowConsole || creation.m_bShowMouseCursor);
    }
    return;
  }
  if (pressed(xiiInputSlot_KeyEscape))
  {
    if (m_bDisplayPreview)
    {
      RevertDisplayPreview();
    }
    else
    {
      m_bShowSettings = false;
      xiiInputManager::SetExclusiveInputSet("");
      return;
    }
  }
  const xiiUInt32 count = 11;
  if (!m_bDisplayPreview)
  {
    if (pressed(xiiInputSlot_KeyDown))
    {
      m_uiSelectedSetting = (m_uiSelectedSetting + 1) % count;
    }
    if (pressed(xiiInputSlot_KeyUp))
    {
      m_uiSelectedSetting = (m_uiSelectedSetting + count - 1) % count;
    }
    const int direction = pressed(xiiInputSlot_KeyRight) ? 1 : pressed(xiiInputSlot_KeyLeft) ? -1 :
                                                                                               0;
    if (direction != 0)
    {
      switch (m_uiSelectedSetting)
      {
        case 0:
          m_PendingWindow.m_WindowMode = static_cast<xiiWindowMode::Enum>((m_PendingWindow.m_WindowMode.GetValue() + 4 + direction) % 4);
          break;
        case 1:
          m_PendingWindow.m_Resolution.width = xiiMath::Clamp(static_cast<int>(m_PendingWindow.m_Resolution.width) + direction * 160, 320, 7680);
          break;
        case 2:
          m_PendingWindow.m_Resolution.height = xiiMath::Clamp(static_cast<int>(m_PendingWindow.m_Resolution.height) + direction * 90, 200, 4320);
          break;
        case 3:
        {
          xiiHybridArray<xiiScreenInfo, 2> screens;
          if (xiiScreen::EnumerateScreens(screens).Succeeded())
          {
            m_PendingWindow.m_iMonitor = static_cast<xiiInt8>(xiiMath::Clamp(static_cast<int>(m_PendingWindow.m_iMonitor) + direction, 0, static_cast<int>(screens.GetCount()) - 1));
          }
          break;
        }
        case 4:
          m_PendingSettings.m_bVSync = !m_PendingSettings.m_bVSync;
          break;
        case 5:
          m_PendingSettings.m_uiFrameLimit = xiiMath::Clamp(static_cast<int>(m_PendingSettings.m_uiFrameLimit) + direction * 10, 0, 1000);
          break;
        case 6:
          m_PendingSettings.m_uiBackgroundFrameLimit = xiiMath::Clamp(static_cast<int>(m_PendingSettings.m_uiBackgroundFrameLimit) + direction * 5, 1, 1000);
          break;
        case 7:
          m_PendingSettings.m_bShowFrameStats = !m_PendingSettings.m_bShowFrameStats;
          break;
        case 8:
          m_PendingSettings.m_fMasterVolume = xiiMath::Clamp(m_PendingSettings.m_fMasterVolume + direction * 0.05f, 0.0f, 1.0f);
          break;
        case 9:
          m_PendingSettings.m_bMuteInBackground = !m_PendingSettings.m_bMuteInBackground;
          break;
        case 10:
          m_PendingWindow.m_bClipMouseCursor = !m_PendingWindow.m_bClipMouseCursor;
          break;
      }
    }
    if (pressed(xiiInputSlot_KeyBackspace))
    {
      xiiGameApplicationSettings defaults;
      defaults.Load(":project/RuntimeConfigs/Presentation.ddl", true).IgnoreResult();
      m_PendingSettings.Apply(defaults).IgnoreResult();
      xiiWindowCreationDescription defaultsWindow;
      defaultsWindow.LoadFromDDL(":project/RuntimeConfigs/Window.ddl").IgnoreResult();
      defaultsWindow.m_Title = m_PendingWindow.m_Title;
      m_PendingWindow        = defaultsWindow;
      m_sSettingsMessage     = "Project defaults restored in the draft. Enter applies.";
    }
  }
  if (pressed(xiiInputSlot_KeyReturn))
  {
    if (m_bDisplayPreview)
    {
      if (m_PendingWindow.SaveToDDL(":appdata/RuntimeConfigs/Window.ddl").Failed() || ApplySettings(m_PendingSettings).Failed())
      {
        m_sSettingsMessage = "Cannot save settings. Check write access to the Player preferences directory.";
      }
      else
      {
        ApplyViewSettings();
        m_bRuntimeSettingsCommitted = true;
        m_sSettingsMessage          = "Display changes confirmed and settings saved.";
      }
      m_bDisplayPreview = false;
    }
    else if (ApplyDisplayPreview().Failed())
    {
      m_sSettingsMessage = "Cannot apply the requested settings. The previous display was restored.";
    }
  }
  if (auto* input = window->GetInputDevice())
  {
    input->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
    input->SetShowMouseCursor(true);
  }
  xiiStringBuilder text("PLAYER SETTINGS  [F2]\n\n");
  const char*      modes[]  = {"Fixed window", "Resizable window", "Borderless fullscreen", "Exclusive fullscreen"};
  const xiiString  labels[] = {"Window mode", "Width", "Height", "Display", "VSync", "Frame limit (0 = unlimited)", "Background FPS", "FPS / frame time", "Master volume", "Mute in background", "Capture cursor"};
  xiiStringBuilder values[count];
  values[0] = modes[m_PendingWindow.m_WindowMode.GetValue()];
  values[1].SetFormat("{}", m_PendingWindow.m_Resolution.width);
  values[2].SetFormat("{}", m_PendingWindow.m_Resolution.height);
  values[3].SetFormat("{}", m_PendingWindow.m_iMonitor);
  values[4] = m_PendingSettings.m_bVSync ? "On" : "Off";
  values[5].SetFormat("{}", m_PendingSettings.m_uiFrameLimit);
  values[6].SetFormat("{}", m_PendingSettings.m_uiBackgroundFrameLimit);
  values[7] = m_PendingSettings.m_bShowFrameStats ? "On" : "Off";
  values[8].SetFormat("{}%%", xiiMath::RoundToInt(m_PendingSettings.m_fMasterVolume * 100));
  values[9]                   = m_PendingSettings.m_bMuteInBackground ? "On" : "Off";
  values[10]                  = m_PendingWindow.m_bClipMouseCursor ? "On" : "Off";
  const auto*     view        = state->GetMainView();
  const float     height      = view ? view->GetViewport().height : static_cast<float>(window->GetClientAreaSize().height);
  const xiiUInt32 visibleRows = xiiMath::Clamp(static_cast<xiiInt32>((height - 190) / 18), 1, static_cast<xiiInt32>(count));
  const xiiUInt32 firstRow    = m_uiSelectedSetting >= visibleRows ? m_uiSelectedSetting - visibleRows + 1 : 0;
  for (xiiUInt32 i = firstRow; i < xiiMath::Min(firstRow + visibleRows, count); ++i)
  {
    text.AppendFormat("{} {}: {}\n", m_uiSelectedSetting == i ? ">" : " ", labels[i], values[i]);
  }
  text.Append("\nArrows: select / change\nEnter: apply  Backspace: defaults\nF2/Escape: close  F1: advanced CVars\nRestart-marked CVars need a restart.\n");
  text.Append(m_sSettingsMessage);
  if (m_bDisplayPreview)
  {
    text.AppendFormat("\nReverting in {} seconds.", xiiMath::Ceil(m_DisplayPreviewDeadline.GetSeconds() - xiiTime::Now().GetSeconds()));
  }
  if (view)
  {
    xiiDebugRenderer::Draw2DRectangle(*view, xiiRectFloat(0, 0, xiiMath::Min(view->GetViewport().width, 760.0f), height), 0.0f, xiiColor(0.025f, 0.03f, 0.045f, 0.96f));
  }
  xiiDebugRenderer::DrawInfoText(state->GetMainWorld(), xiiDebugTextPlacement::TopLeft, "PlayerSettings", text, xiiColor::White);
}

void xiiPlayerApplication::BeforeHighLevelSystemsShutdown()
{
  ReportStatus("stopping", "Releasing scene and application services");
  SUPER::BeforeHighLevelSystemsShutdown();
  // CVar shutdown persistence must not turn explicit launch overrides into next-run defaults.
  if (!m_bRuntimeSettingsCommitted)
  {
    m_PersistedSettings.Load(":appdata/RuntimeConfigs/Presentation.ddl", m_PersistedSettings.m_bVSync).IgnoreResult();
    m_bSettingsInitialized = false;
    const auto* command    = xiiCommandLineUtils::GetGlobalInstance();
    if (command->HasOption("-App.VSync"))
    {
      cvar_AppVSync = m_PersistedSettings.m_bVSync;
    }
    if (command->HasOption("-App.FrameLimit"))
    {
      if (auto* cap = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit")))
      {
        *cap = static_cast<xiiInt32>(m_PersistedSettings.m_uiFrameLimit);
      }
    }
  }
}

void xiiPlayerApplication::BeforeCoreSystemsShutdown()
{
  m_pStartupWindow.Clear();
  SUPER::BeforeCoreSystemsShutdown();
  xiiGlobalLog::RemoveLogWriter(m_LogSubscription);
  m_Log.EndLog();
}

void xiiPlayerApplication::AfterCoreSystemsShutdown()
{
  m_UserSettingsLock.Close();
  m_TelemetryPortLease.Close();
  if (!m_sOwnedTemporaryDirectory.IsEmpty() && xiiOSFile::DeleteFolder(m_sOwnedTemporaryDirectory).Failed())
  {
    xiiLog::Warning("Cannot remove Player temporary directory '{}'.", m_sOwnedTemporaryDirectory);
  }
  SUPER::AfterCoreSystemsShutdown();
}

void xiiPlayerApplication::DetermineProjectPath()
{
  xiiStringBuilder sProjectPath = opt_Project.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by xiiFileserve.
  // xiiFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../XII/Data/Samples/SampleGame

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    xiiStringBuilder packaged(xiiOSFile::GetApplicationDirectory(), "/../Data/project");
    packaged.MakeCleanPath();
    if (xiiOSFile::ExistsFile(xiiStringBuilder(packaged, "/xiiProject")))
    {
      sProjectPath = packaged;
    }
  }

  if (sProjectPath.IsEmpty())
  {
    const xiiStringBuilder sScenePath = opt_Scene.GetOptionValue(xiiCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (xiiFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "xiiProject", "xiiSdkRoot.txt").Failed())
    {
      // couldn't find the 'xiiProject' file in any parent folder of the scene
      m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!xiiPathUtils::IsAbsolutePath(sProjectPath))
  {
    xiiStringBuilder executableRelative(xiiOSFile::GetApplicationDirectory(), "/", sProjectPath);
    executableRelative.MakeCleanPath();
    if (xiiOSFile::ExistsDirectory(executableRelative))
    {
      sProjectPath = executableRelative;
    }
    else
    {
      sProjectPath.Prepend(xiiFileSystem::GetSdkRootDirectory(), "/");
    }
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/xiiProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = xiiFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;
}
