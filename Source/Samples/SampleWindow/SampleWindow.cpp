#include <SampleWindow/SampleWindow.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

class xiiSampleWindow : public xiiWindow
{
public:
  xiiSampleWindow() :
    xiiWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void       OnClickClose() override { m_bCloseRequested = true; }
  virtual xiiSizeU32 GetClientAreaSize() const override { return xiiSizeU32(g_uiWindowWidth, g_uiWindowHeight); }
  virtual void       OnResize(const xiiSizeU32& newWindowSize) override
  {
    if (g_uiWindowWidth != newWindowSize.width || g_uiWindowHeight != newWindowSize.height)
    {
      g_uiWindowWidth  = newWindowSize.width;
      g_uiWindowHeight = newWindowSize.height;
      g_bWindowResized = true;
    }
  }

  bool m_bCloseRequested;
};

xiiSampleWindowApp::xiiSampleWindowApp() :
  xiiApplication("xiiSampleWindow")
{
}

xiiApplication::Execution xiiSampleWindowApp::Run()
{
  m_pWindow->ProcessWindowMessages();

  if (g_bWindowResized)
  {
    g_bWindowResized = false;
  }

  if (m_pWindow->m_bCloseRequested || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
    return Execution::Quit;

  // Make sure time goes on
  xiiClock::GetGlobalClock()->Update();

  // Update all input state
  xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

  // Engage mouse look
  if (xiiInputManager::GetInputActionState("Main", "Look") == xiiKeyState::Down)
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::ClipToPosition);

    float       fInputValue = 0.0f;
    const float fMouseSpeed = 0.01f;

    xiiVec3 mouseMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "LookPosX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x += fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookNegX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x -= fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookPosY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y -= fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookNegY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y += fInputValue * fMouseSpeed;
  }
  else
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
  }

  // Turn camera with arrow keys
  {
    float       fInputValue = 0.0f;
    const float fTurnSpeed  = 1.0f;

    xiiVec3 mouseMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "TurnPosX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x += fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnNegX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x -= fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnPosY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y += fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnNegY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y -= fInputValue * fTurnSpeed;
  }

  // Apply translation
  {
    float   fInputValue = 0.0f;
    xiiVec3 cameraMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != xiiKeyState::Up)
      cameraMotion.x += fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != xiiKeyState::Up)
      cameraMotion.x -= fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != xiiKeyState::Up)
      cameraMotion.y += fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != xiiKeyState::Up)
      cameraMotion.y -= fInputValue;
  }

  // Make sure telemetry is sent out regularly.
  xiiTelemetry::PerFrameUpdate();

  // Needs to be called once per frame
  xiiResourceManager::PerFrameUpdate();

  // Tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  xiiTaskSystem::FinishFrameTasks();

  return xiiApplication::Execution::Continue;
}

void xiiSampleWindowApp::AfterCoreSystemsStartup()
{
  xiiStringBuilder sProjectDir = ">sdk/Data/Samples/SampleWindow";
  xiiStringBuilder sProjectDirResolved;
  xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

  xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
  xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
  xiiTelemetry::SetServerName("Sample Window");

  // Activate xiiTelemetry such that the inspector plugin can use the network connection.
  xiiTelemetry::CreateServer();

  // Load the inspector plugin.
  // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
  // When you are using xiiApplication, this is done automatically.
  xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();
#endif

  // Register Input
  {
    xiiInputActionConfig cfg;

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
    xiiInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookPosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosX;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookPosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegX;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookPosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosY;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookPosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegY;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookNegY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnPosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyRight;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnPosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyLeft;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnPosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyDown;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnPosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyUp;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnNegY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling    = false;
    xiiInputManager::SetInputActionConfig("Main", "Look", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyD;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyA;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyW;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyS;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);
  }

  // Create a window for rendering
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    WindowCreationDesc.m_Title             = "Sample Window";
    WindowCreationDesc.m_bShowMouseCursor  = true;
    WindowCreationDesc.m_bClipMouseCursor  = false;
    WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
    m_pWindow                              = XII_DEFAULT_NEW(xiiSampleWindow);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
  }

  // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
  xiiStartup::StartupHighLevelSystems();
}

void xiiSampleWindowApp::BeforeCoreSystemsShutdown()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
  // Shut down telemetry if it was set up.
  xiiTelemetry::CloseConnection();
#endif

  SUPER::BeforeCoreSystemsShutdown();
}

void xiiSampleWindowApp::BeforeHighLevelSystemsShutdown()
{
  // Tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  xiiStartup::ShutdownHighLevelSystems();

  // Finally destroy the window
  m_pWindow->Destroy().IgnoreResult();
  XII_DEFAULT_DELETE(m_pWindow);
}

XII_CONSOLEAPP_ENTRY_POINT(xiiSampleWindowApp);
