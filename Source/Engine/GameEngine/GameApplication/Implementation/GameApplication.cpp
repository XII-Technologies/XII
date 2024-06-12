#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
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
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

xiiGameApplication*                                                xiiGameApplication::s_pGameApplicationInstance = nullptr;
xiiDelegate<xiiGALDevice*(const xiiGALDeviceCreationDescription&)> xiiGameApplication::s_DefaultDeviceCreator;

xiiCVarBool xiiGameApplication::cvar_AppVSync("App.VSync", false, xiiCVarFlags::Save, "Enables V-Sync");
xiiCVarBool xiiGameApplication::cvar_AppShowFPS("App.ShowFPS", false, xiiCVarFlags::Save, "Show frames per second counter");

xiiGameApplication::xiiGameApplication(xiiStringView sAppName, xiiStringView sProjectPath /*= {}*/) :
  xiiGameApplicationBase(sAppName), m_sAppProjectPath(sProjectPath)
{
  m_pUpdateTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "UpdateWorldsAndExtractViews", xiiTaskNesting::Never, xiiMakeDelegate(&xiiGameApplication::UpdateWorldsAndExtractViews, this));
  m_pUpdateTask->ConfigureTask("GameApplication.Update", xiiTaskNesting::Maybe);

  s_pGameApplicationInstance = this;
  m_bWasQuitRequested        = false;

  m_pConsole = XII_DEFAULT_NEW(xiiQuakeConsole);
  xiiConsole::SetMainConsole(m_pConsole.Borrow());
}

xiiGameApplication::~xiiGameApplication()
{
  s_pGameApplicationInstance = nullptr;
}

// static
void xiiGameApplication::SetOverrideDefaultDeviceCreator(xiiDelegate<xiiGALDevice*(const xiiGALDeviceCreationDescription&)> creator)
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
    xiiStringBuilder relToSdk(">sdk/", m_sAppProjectPath);
    xiiStringBuilder absToSdk;
    if (xiiFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (xiiOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

  xiiStringBuilder result;
  if (xiiFileSystem::FindFolderWithSubPath(result, xiiOSFile::GetApplicationDirectory(), m_sAppProjectPath).Failed())
  {
    xiiLog::Error("Could not find the project directory.");
  }

  return result;
}

bool xiiGameApplication::IsGameUpdateEnabled() const
{
  return xiiRenderWorld::GetMainViews().GetCount() > 0;
}

void xiiGameApplication::Run_WorldUpdateAndRender()
{
  XII_PROFILE_SCOPE("Run_WorldUpdateAndRender");
  // If multi-threaded rendering is disabled, the same content is updated/extracted and rendered in the same frame.
  // As xiiRenderWorld::BeginFrame applies the render pipeline properties that were set during the update phase, it needs to be done after update/extraction but before rendering.
  if (!xiiRenderWorld::GetUseMultithreadedRendering())
  {
    UpdateWorldsAndExtractViews();
  }

  xiiRenderWorld::BeginFrame();

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // On most platforms it doesn't matter that much how early this happens.
  // But on HoloLens this executes something that needs to be done at the right time,
  // for the reprojection to work properly.
  const xiiUInt64 uiRenderFrame = xiiRenderWorld::GetUseMultithreadedRendering() ? xiiRenderWorld::GetFrameCounter() - 1 : xiiRenderWorld::GetFrameCounter();
  pDevice->BeginFrame(uiRenderFrame);

  xiiTaskGroupID updateTaskID;
  if (xiiRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = xiiTaskSystem::StartSingleTask(m_pUpdateTask, xiiTaskPriority::EarlyThisFrame);
  }

  RenderFps();
  RenderConsole();

  xiiRenderWorld::Render(xiiRenderContext::GetDefaultInstance());

  if (xiiRenderWorld::GetUseMultithreadedRendering())
  {
    XII_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    xiiTaskSystem::WaitForGroup(updateTaskID);
  }
}

void xiiGameApplication::Run_Present()
{
  xiiHybridArray<xiiActor*, 8> allActors;
  xiiActorManager::GetSingleton()->GetAllActors(allActors);

  for (xiiActor* pActor : allActors)
  {
    XII_PROFILE_SCOPE(pActor->GetName());

    xiiActorPluginWindow* pWindowPlugin = pActor->GetPlugin<xiiActorPluginWindow>();

    if (pWindowPlugin == nullptr)
      continue;

    // Ignore actors without an output target
    if (auto pOutput = pWindowPlugin->GetOutputTarget())
    {
      // if we have multiple actors, append the actor name to each screenshot
      xiiStringBuilder ctxt;
      if (allActors.GetCount() > 1)
      {
        ctxt.Append(" - ", pActor->GetName());
      }

      ExecuteTakeScreenshot(pOutput, ctxt);

      if (pWindowPlugin->GetWindow())
      {
        ExecuteFrameCapture(pWindowPlugin->GetWindow()->GetNativeWindowHandle(), ctxt);
      }

      XII_PROFILE_SCOPE("Present");
      pOutput->Present(cvar_AppVSync);
    }
  }
}

void xiiGameApplication::Run_FinishFrame()
{
  xiiGALDevice::GetDefaultDevice()->EndFrame();
  xiiRenderWorld::EndFrame();

  SUPER::Run_FinishFrame();
}

void xiiGameApplication::UpdateWorldsAndExtractViews()
{
  xiiStringBuilder sb;
  sb.SetFormat("FRAME {}", xiiRenderWorld::GetFrameCounter());
  XII_PROFILE_SCOPE(sb.GetData());

  Run_BeforeWorldUpdate();

  static xiiHybridArray<xiiWorld*, 16> worldsToUpdate;
  worldsToUpdate.Clear();

  auto mainViews = xiiRenderWorld::GetMainViews();
  for (auto hView : mainViews)
  {
    xiiView* pView = nullptr;
    if (xiiRenderWorld::TryGetView(hView, pView))
    {
      xiiWorld* pWorld = pView->GetWorld();

      if (pWorld != nullptr && !worldsToUpdate.Contains(pWorld))
      {
        worldsToUpdate.PushBack(pWorld);
      }
    }
  }

  if (xiiRenderWorld::GetUseMultithreadedRendering())
  {
    xiiTaskGroupID updateWorldsTaskID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
    for (xiiUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      xiiTaskSystem::AddTaskToGroup(updateWorldsTaskID, worldsToUpdate[i]->GetUpdateTask());
    }
    xiiTaskSystem::StartTaskGroup(updateWorldsTaskID);
    xiiTaskSystem::WaitForGroup(updateWorldsTaskID);
  }
  else
  {
    for (xiiUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      xiiWorld* pWorld = worldsToUpdate[i];
      XII_LOCK(pWorld->GetWriteMarker());

      pWorld->Update();
    }
  }

  Run_AfterWorldUpdate();

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  xiiRenderWorld::ExtractMainViews();
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

  if (cvar_AppShowFPS)
  {
    if (const xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView))
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

void xiiGameApplication::RenderConsole()
{
  XII_PROFILE_SCOPE("RenderConsole");

  if (!m_bShowConsole || !m_pConsole)
    return;

  const xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  xiiViewHandle hView = pView->GetHandle();

  const float fViewWidth             = pView->GetViewport().width;
  const float fViewHeight            = pView->GetViewport().height;
  const float fTextHeight            = 20.0f;
  const float fConsoleHeight         = (fViewHeight / 2.0f);
  const float fBorderWidth           = 3.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - fTextHeight - (2.0f * fBorderWidth);

  const xiiInt32 iTextHeight = (xiiInt32)fTextHeight;
  const xiiInt32 iTextLeft   = (xiiInt32)(fBorderWidth);

  {
    xiiColor backgroundColor(0.0f, 0.0f, 0.0f, 0.7f);
    xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

    xiiColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f, foregroundColor);
  }

  {
    XII_LOCK(m_pConsole->GetMutex());

    auto& consoleStrings = m_pConsole->GetConsoleStrings();

    xiiUInt32 uiNumConsoleLines = (xiiUInt32)(xiiMath::Ceil(fConsoleTextAreaHeight / fTextHeight));
    xiiInt32  iFirstLinePos     = (xiiInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
    xiiInt32  uiFirstLine       = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
    xiiInt32  uiSkippedLines    = xiiMath::Max(uiFirstLine - (xiiInt32)consoleStrings.GetCount() + 1, 0);

    for (xiiUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      xiiDebugRenderer::Draw2DText(hView, consoleString.m_sText.GetData(), xiiVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
    }

    xiiDebugRenderer::Draw2DText(hView, m_pConsole->GetInputLine(), xiiVec2I32(iTextLeft, (xiiInt32)(fConsoleTextAreaHeight + fBorderWidth)), xiiColor::White);

    if (xiiMath::Fraction(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float    fCaretPosition = (float)m_pConsole->GetCaretPosition();
      xiiColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      xiiDebugRenderer::Draw2DRectangle(hView, xiiRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f), 0.0f, caretColor);
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
    cvar_AppShowFPS = !cvar_AppShowFPS;
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
