#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorManager.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

xiiGameApplicationBase* xiiGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

xiiGameApplicationBase::xiiGameApplicationBase(xiiStringView sAppName) :
  xiiApplication(sAppName), m_ConFunc_TakeScreenshot("TakeScreenshot", "()", xiiMakeDelegate(&xiiGameApplicationBase::TakeScreenshot, this)), m_ConFunc_CaptureFrame("CaptureFrame", "()", xiiMakeDelegate(&xiiGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

xiiGameApplicationBase::~xiiGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(xiiStringBuilder& out_sString)
{
  const xiiDateTime dt = xiiTimestamp::CurrentTimestamp();

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), xiiArgU(dt.GetMonth(), 2, true), xiiArgU(dt.GetDay(), 2, true), xiiArgU(dt.GetHour(), 2, true), xiiArgU(dt.GetMinute(), 2, true), xiiArgU(dt.GetSecond(), 2, true), xiiArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void xiiGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public xiiTask
  {
  public:
    xiiProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask()  = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      xiiStringBuilder sPath(":appdata/Profiling/", xiiApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      xiiFileWriter fileWriter;
      if (fileWriter.Open(sPath) == XII_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        xiiLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        xiiLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  xiiSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = XII_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", xiiTaskNesting::Never);
  xiiProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  xiiTaskSystem::StartSingleTask(pWriteProfilingDataTask, xiiTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void xiiGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void xiiGameApplicationBase::StoreScreenshot(xiiImage&& image, xiiStringView sContext /*= {} */)
{
  class WriteFileTask final : public xiiTask
  {
  public:
    xiiImage         m_Image;
    xiiStringBuilder m_sPath;

    WriteFileTask()  = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // Remove the Alpha channel before saving
      m_Image.Convert(xiiImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        xiiLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  xiiSharedPtr<WriteFileTask> pWriteTask = XII_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", xiiTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.SetFormat(":appdata/Screenshots/{0}", xiiApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(sContext);
  pWriteTask->m_sPath.Append(".png");

  // We move the file writing off to another thread to save some time.
  // If we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  xiiTaskSystem::StartSingleTask(pWriteTask, xiiTaskPriority::LongRunning);
}

void xiiGameApplicationBase::ExecuteTakeScreenshot(xiiWindowOutputTargetBase* pOutputTarget, xiiStringView sContext /* = {} */)
{
  if (m_bTakeScreenshot)
  {
    XII_PROFILE_SCOPE("ExecuteTakeScreenshot");
    xiiImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(std::move(img), sContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void xiiGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool xiiGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


xiiResult xiiGameApplicationBase::GetAbsFrameCaptureOutputPath(xiiStringBuilder& ref_sOutputPath)
{
  xiiStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return xiiFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void xiiGameApplicationBase::ExecuteFrameCapture(xiiWindowHandle targetWindowHandle, xiiStringView sContext /*= {} */)
{
  xiiFrameCaptureInterface* pCaptureInterface = xiiSingletonRegistry::GetSingletonInstance<xiiFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  XII_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      xiiStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(sContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      xiiStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        xiiLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        xiiLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiGameApplicationBase::ActivateGameState(xiiWorld* pWorld /*= nullptr*/, const xiiTransform* pStartPosition /*= nullptr*/)
{
  XII_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState(pWorld);

  if (m_pGameState == nullptr)
    return XII_FAILURE;

  m_pWorldLinkedWithGameState = pWorld;
  m_pGameState->OnActivation(pWorld, pStartPosition);

  xiiGameApplicationStaticEvent e;
  e.m_Type = xiiGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  XII_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());

  return XII_SUCCESS;
}

void xiiGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  XII_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  xiiGameApplicationStaticEvent e;
  e.m_Type = xiiGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  xiiActorManager::GetSingleton()->DestroyAllActors(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

xiiGameStateBase* xiiGameApplicationBase::GetActiveGameStateLinkedToWorld(const xiiWorld* pWorld) const
{
  if (m_pWorldLinkedWithGameState == pWorld)
    return m_pGameState.Borrow();

  return nullptr;
}

xiiUniquePtr<xiiGameStateBase> xiiGameApplicationBase::CreateGameState(xiiWorld* pWorld)
{
  XII_LOG_BLOCK("Create Game State");

  xiiUniquePtr<xiiGameStateBase> pCurState;

  {
    xiiInt32 iBestPriority = -1;

    xiiRTTI::ForEachDerivedType<xiiGameStateBase>(
      [&](const xiiRTTI* pRtti) {
        xiiUniquePtr<xiiGameStateBase> pState = pRtti->GetAllocator()->Allocate<xiiGameStateBase>();

        const xiiInt32 iPriority = (xiiInt32)pState->DeterminePriority(pWorld);
        if (iPriority > iBestPriority)
        {
          iBestPriority = iPriority;

          pCurState = std::move(pState);
        }
      },
      xiiRTTI::ForEachOptions::ExcludeNonAllocatable);
  }

  return pCurState;
}

void xiiGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState().IgnoreResult();
}

xiiResult xiiGameApplicationBase::BeforeCoreSystemsStartup()
{
  xiiStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void xiiGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (WasQuitRequested())
  {
    return;
  }

  xiiStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void xiiGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void xiiGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // Ensure that no resources continue to be streamed in, while the engine shuts down
    xiiResourceManager::EngineAboutToShutdown();
    xiiResourceManager::ExecuteAllResourceCleanupCallbacks();
    xiiResourceManager::FreeAllUnusedResources();
  }
}

void xiiGameApplicationBase::BeforeCoreSystemsShutdown()
{
  // Shut down all actors and APIs that may have been in use
  if (xiiActorManager::GetSingleton() != nullptr)
  {
    xiiActorManager::GetSingleton()->Shutdown();
  }

  {
    xiiFrameAllocator::Reset();
    xiiResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    xiiResourceManager::FreeAllUnusedResources();
  }

  Deinit_UnloadPlugins();

  // Shut down telemetry if it was set up
  {
    xiiTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

XII_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  s_bUpdatePluginsExecuted = true;
}

xiiApplication::Execution xiiGameApplicationBase::Run()
{
  if (m_bWasQuitRequested)
    return xiiApplication::Execution::Quit;

  RunOneFrame();

  return xiiApplication::Execution::Continue;
}

void xiiGameApplicationBase::RunOneFrame()
{
  XII_PROFILE_SCOPE("Run");
  s_bUpdatePluginsExecuted = false;

  xiiActorManager::GetSingleton()->Update();

  if (!IsGameUpdateEnabled())
    return;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    XII_PROFILE_SCOPE("GameApp_BeginAppTick");
    XII_BROADCAST_EVENT(GameApp_BeginAppTick);
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_InputUpdate();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    XII_ASSERT_DEV(s_bUpdatePluginsExecuted, "xiiGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                             "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // For plugins that need to hook into this without a link dependency on this lib
    XII_PROFILE_SCOPE("GameApp_EndAppTick");
    XII_BROADCAST_EVENT(GameApp_EndAppTick);

    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    XII_PROFILE_SCOPE("BeforePresent");
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    XII_PROFILE_SCOPE("Run_Present");
    Run_Present();
  }
  xiiClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    XII_PROFILE_SCOPE("AfterPresent");
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    XII_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
}

void xiiGameApplicationBase::Run_InputUpdate()
{
  XII_PROFILE_SCOPE("Run_InputUpdate");
  xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool xiiGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void xiiGameApplicationBase::Run_BeforeWorldUpdate()
{
  XII_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void xiiGameApplicationBase::Run_AfterWorldUpdate()
{
  XII_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();
  }

  {
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void xiiGameApplicationBase::Run_UpdatePlugins()
{
  XII_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // For plugins that need to hook into this without a link dependency on this lib
  XII_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    xiiGameApplicationExecutionEvent e;
    e.m_Type = xiiGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void xiiGameApplicationBase::Run_Present() {}

void xiiGameApplicationBase::Run_FinishFrame()
{
  xiiTelemetry::PerFrameUpdate();
  xiiResourceManager::PerFrameUpdate();
  xiiTaskSystem::FinishFrameTasks();
  xiiFrameAllocator::Swap();
  xiiProfilingSystem::StartNewFrame();

  // If many messages have been logged, make sure they get written to disk
  xiiLog::Flush(100, xiiTime::MakeFromSeconds(10));

  // Reset this state
  m_bTakeScreenshot = false;
}

void xiiGameApplicationBase::UpdateFrameTime()
{
  // Do not use xiiClock for this, it smooths and clamps the timestep
  const xiiTime tNow = xiiTime::Now();

  static xiiTime tLast = tNow;
  m_FrameTime          = tNow - tLast;
  tLast                = tNow;
}

XII_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
