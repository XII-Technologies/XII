/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>
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
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

xiiGameApplication*                                                             xiiGameApplication::s_pGameApplicationInstance = nullptr;
xiiDelegate<xiiSharedPtr<xiiGALDevice>(const xiiGALDeviceCreationDescription&)> xiiGameApplication::s_DefaultDeviceCreator;

xiiCVarBool xiiGameApplication::cvar_AppVSync("App.VSync", true, xiiCVarFlags::Save, "Synchronizes frame presentation with the display refresh to reduce or eliminate screen tearing. This may introduce input latency and cap the framerate to the monitor's refresh rate. Useful for visual fidelity and stable presentation.");
xiiCVarBool xiiGameApplication::cvar_AppShowFrameStats("App.ShowFrameStats", false, xiiCVarFlags::Save, "Displays a live frames-stats overlay on-screen to aid profiling and spot frame-time spikes during development and debugging. Provides an immediate performance readout without affecting rendering state.");

xiiGameApplication::xiiGameApplication(xiiStringView sAppName, xiiStringView sProjectPath /*= {}*/) :
  xiiGameApplicationBase(sAppName), m_sAppProjectPath(sProjectPath)
{
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested        = false;

  m_pConsole = XII_DEFAULT_NEW(xiiQuakeConsole);
  xiiConsole::SetMainConsole(m_pConsole.Borrow());

  m_CVarChangeSubscriptionID = cvar_AppVSync.m_CVarEvents.AddEventHandler(xiiMakeDelegate(&xiiGameApplication::OnVSyncChanged, this));
}

xiiGameApplication::~xiiGameApplication()
{
  s_pGameApplicationInstance = nullptr;

  cvar_AppVSync.m_CVarEvents.RemoveEventHandler(m_CVarChangeSubscriptionID);
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

  Run_BeforeWorldUpdate();

  xiiTaskGroupID updateWorldsTaskID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
  for (xiiUInt32 i = 0; i < xiiWorld::GetWorldCount(); ++i)
  {
    xiiWorld* pWorld = xiiWorld::GetWorld(i);

    if (pWorld->GetWorldSimulationEnabled())
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
}

void xiiGameApplication::RenderFps()
{
  XII_PROFILE_SCOPE("RenderFps");
  // Do not use xiiClock for this, it smooths and clamps the timestep

  static xiiTime   tAccumTime;
  static xiiTime   tDisplayedFrameTime = m_FrameTime;
  static xiiUInt32 uiFrames            = 0;
  static xiiUInt32 uiFPS               = 0;

  ++uiFrames;
  tAccumTime += m_FrameTime;

  if (tAccumTime >= xiiTime::MakeFromSeconds(0.5))
  {
    tAccumTime -= xiiTime::MakeFromSeconds(0.5);
    tDisplayedFrameTime = m_FrameTime;

    uiFPS    = uiFrames * 2;
    uiFrames = 0;
  }

  if (cvar_AppShowFrameStats)
  {
    if (xiiGameState* pGameState = xiiDynamicCast<xiiGameState*>(m_pGameState.Borrow()))
    {
      if (xiiWorld* pMainWorld = pGameState->GetMainWorld())
      {
        xiiRenderWorldModule* pRenderWorldModule = pMainWorld->GetOrCreateModule<xiiRenderWorldModule>();

        if (const xiiView* pView = pRenderWorldModule->GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView))
        {
          if (uiFPS >= 60)
          {
            xiiDebugRenderer::DrawInfoText(pView->GetHandle(), xiiDebugTextPlacement::BottomLeft, "FPS", xiiFmt("{0} FPS, {1} ms", uiFPS, xiiArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)), xiiColor::Green);
          }
          else if (uiFPS >= 30)
          {
            xiiDebugRenderer::DrawInfoText(pView->GetHandle(), xiiDebugTextPlacement::BottomLeft, "FPS", xiiFmt("{0} FPS, {1} ms", uiFPS, xiiArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)), xiiColor::LightGreen);
          }
          else if (uiFPS >= 15)
          {
            xiiDebugRenderer::DrawInfoText(pView->GetHandle(), xiiDebugTextPlacement::BottomLeft, "FPS", xiiFmt("{0} FPS, {1} ms", uiFPS, xiiArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)), xiiColor::Yellow);
          }
          else
          {
            xiiDebugRenderer::DrawInfoText(pView->GetHandle(), xiiDebugTextPlacement::BottomLeft, "FPS", xiiFmt("{0} FPS, {1} ms", uiFPS, xiiArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)), xiiColor::Red);
          }
        }
      }
    }
  }
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
      xiiRenderWorldModule* pRenderWorldModule = pMainWorld->GetOrCreateModule<xiiRenderWorldModule>();

      if (const xiiView* pView = pRenderWorldModule->GetViewByUsageHint(xiiCameraUsageHint::MainView))
      {
        xiiViewHandle hView = pView->GetHandle();

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
          xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

          xiiColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
          xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
          xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fLineHeight), 0.0f, foregroundColor);
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
            xiiDebugRenderer::Draw2DText(hView, consoleString.m_sText.GetData(), xiiVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
          }

          xiiDebugRenderer::Draw2DText(hView, m_pConsole->GetInputLine(), xiiVec2I32(iTextLeft, (xiiInt32)(fConsoleTextAreaHeight + fBorderWidth + (fLineHeight * 0.5f))), xiiColor::White, 16, xiiDebugTextHAlign::Default, xiiDebugTextVAlign::Center);

          if (xiiMath::Fraction(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
          {
            const float fCaretPosition = (float)m_pConsole->GetCaretPosition();
            const float fCaretX        = fBorderWidth + (fCaretPosition + 0.5f) * fGlyphWidth;
            const float fCaretY        = fConsoleTextAreaHeight + fBorderWidth + 1.0f;
            xiiColor    caretColor(1.0f, 1.0f, 1.0f, 0.5f);
            xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fCaretX, fCaretY, 2.0f, fLineHeight - 2.0f), 0.0f, caretColor);
          }
        }
      }
    }
  }
}

void xiiGameApplication::OnVSyncChanged(const xiiCVarEvent& e)
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
      pOutput->SetVSyncEnabled(cvar_AppVSync);
    }
  }
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
    cvar_AppShowFrameStats = !cvar_AppShowFrameStats;
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
