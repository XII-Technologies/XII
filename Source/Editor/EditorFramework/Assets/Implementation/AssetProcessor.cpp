#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

XII_IMPLEMENT_SINGLETON(xiiAssetProcessor);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiAssetProcessor* pDummy = xiiAssetProcessor::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiCuratorLog
////////////////////////////////////////////////////////////////////////

void xiiAssetProcessorLog::HandleLogMessage(const xiiLoggingEventData& le)
{
  m_LoggingEvent.Broadcast(le);
}

void xiiAssetProcessorLog::AddLogWriter(xiiLoggingEvent::Handler handler)
{
  m_LoggingEvent.AddEventHandler(handler);
}

void xiiAssetProcessorLog::RemoveLogWriter(xiiLoggingEvent::Handler handler)
{
  m_LoggingEvent.RemoveEventHandler(handler);
}


////////////////////////////////////////////////////////////////////////
// xiiAssetProcessor
////////////////////////////////////////////////////////////////////////

xiiAssetProcessor::xiiAssetProcessor() :
  m_SingletonRegistrar(this)
{
}

xiiAssetProcessor::~xiiAssetProcessor()
{
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }
  XII_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Call StopProcessTask first before destroying the xiiAssetProcessor.");
}

void xiiAssetProcessor::StartProcessTask()
{
  XII_LOCK(m_ProcessorMutex);
  if (m_ProcessTaskState != ProcessTaskState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }

  m_ProcessTaskState = ProcessTaskState::Running;

  const xiiUInt32 uiWorkerCount = xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::LongTasks);
  m_ProcessTasks.SetCount(uiWorkerCount);

  for (xiiUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_ProcessTasks[idx].m_uiProcessorID = idx;
  }

  m_pThread = XII_DEFAULT_NEW(xiiProcessThread);
  m_pThread->Start();

  {
    xiiAssetProcessorEvent e;
    e.m_Type = xiiAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void xiiAssetProcessor::StopProcessTask(bool bForce)
{
  {
    XII_LOCK(m_ProcessorMutex);
    switch (m_ProcessTaskState)
    {
      case ProcessTaskState::Running:
      {
        m_ProcessTaskState = ProcessTaskState::Stopping;
        {
          xiiAssetProcessorEvent e;
          e.m_Type = xiiAssetProcessorEvent::Type::ProcessTaskStateChanged;
          m_Events.Broadcast(e);
        }
      }
      break;
      case ProcessTaskState::Stopping:
        if (!bForce)
          return;
        break;
      default:
      case ProcessTaskState::Stopped:
        return;
    }
  }

  if (bForce)
  {
    m_bForceStop = true;
    m_pThread->Join();
    m_pThread.Clear();
    XII_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Process task shoul have set the state to stopped.");
  }
}

void xiiAssetProcessor::AddLogWriter(xiiLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void xiiAssetProcessor::RemoveLogWriter(xiiLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void xiiAssetProcessor::Run()
{
  while (m_ProcessTaskState == ProcessTaskState::Running)
  {
    for (xiiUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      m_ProcessTasks[i].Tick(true);
    }
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
  }

  while (true)
  {
    bool bAnyRunning = false;

    for (xiiUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_bForceStop)
        m_ProcessTasks[i].ShutdownProcess();

      bAnyRunning |= m_ProcessTasks[i].Tick(false);
    }

    if (bAnyRunning)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
    else
      break;
  }

  XII_LOCK(m_ProcessorMutex);
  m_ProcessTasks.Clear();
  m_ProcessTaskState = ProcessTaskState::Stopped;
  m_bForceStop       = false;
  {
    xiiAssetProcessorEvent e;
    e.m_Type = xiiAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}


////////////////////////////////////////////////////////////////////////
// xiiProcessTask
////////////////////////////////////////////////////////////////////////

xiiProcessTask::xiiProcessTask() :
  m_Status(XII_SUCCESS)
{
  m_pIPC = XII_DEFAULT_NEW(xiiEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(xiiMakeDelegate(&xiiProcessTask::EventHandlerIPC, this));
}

xiiProcessTask::~xiiProcessTask()
{
  ShutdownProcess();
  m_pIPC->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiProcessTask::EventHandlerIPC, this));
  XII_DEFAULT_DELETE(m_pIPC);
}

xiiResult xiiProcessTask::StartProcess()
{
  const xiiRTTI* pFirstAllowedMessageType = nullptr;

  xiiStringBuilder tmp;

  QStringList args;
  args << "-appname";
  args << xiiApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << xiiToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << xiiGameApplication::GetActiveRenderer().GetData(tmp);

  {
    xiiStringBuilder sRelativeData;
    sRelativeData = ":APPDATA";

    xiiStringBuilder sAbsoluteData;
    xiiFileSystem::ResolvePath(sRelativeData, &sAbsoluteData, nullptr).AssertSuccess("Failed to resolve APPDATA dir!");

    args << "-outputDir";
    args << sAbsoluteData.GetData();
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "xiiEditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "xiiEditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiProcessTask::ShutdownProcess()
{
  m_pIPC->CloseConnection();
}

void xiiProcessTask::EventHandlerIPC(const xiiProcessCommunicationChannel::Event& e)
{
  if (const xiiProcessAssetResponseMsg* pMsg = xiiDynamicCast<const xiiProcessAssetResponseMsg*>(e.m_pMessage))
  {
    XII_ASSERT_DEV(m_State == State::Processing, "Message handling should only happen when currently processing");
    m_Status = pMsg->m_Status;
    m_State  = State::ReportResult;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool xiiProcessTask::GetNextAssetToProcess(xiiAssetInfo* pInfo, xiiUuid& out_guid, xiiDataDirPath& out_path)
{
  bool bComplete = true;

  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(pInfo->m_Path, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const xiiAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(xiiAssetDocumentFlags::OnlyTransformManually | xiiAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const xiiSet<xiiString>& files) -> xiiAssetInfo* {
    for (const auto& sFile : files)
    {
      if (xiiAssetInfo* pFileInfo = xiiAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case xiiAssetInfo::TransformState::Unknown:
          case xiiAssetInfo::TransformState::TransformError:
          case xiiAssetInfo::TransformState::MissingTransformDependency:
          case xiiAssetInfo::TransformState::MissingPackageDependency:
          case xiiAssetInfo::TransformState::MissingThumbnailDependency:
          case xiiAssetInfo::TransformState::CircularDependency:
          {
            bComplete = false;
            continue;
          }
          case xiiAssetInfo::TransformState::NeedsTransform:
          case xiiAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
          }
          case xiiAssetInfo::TransformState::UpToDate:
            continue;

          case xiiAssetInfo::TransformState::NeedsImport:
            // the main processor has to do this itself
            continue;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (xiiAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_TransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  if (xiiAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_ThumbnailDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  // not needed to go through package dependencies here

  if (bComplete && !xiiAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !xiiAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    xiiAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_path = pInfo->m_Path;
    return true;
  }

  return false;
}

bool xiiProcessTask::GetNextAssetToProcess(xiiUuid& out_guid, xiiDataDirPath& out_path)
{
  XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = xiiAssetCurator::GetSingleton()->m_TransformState[xiiAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    xiiAssetInfo* pInfo = xiiAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  for (auto it = xiiAssetCurator::GetSingleton()->m_TransformState[xiiAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    xiiAssetInfo* pInfo = xiiAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  return false;
}

void xiiProcessTask::OnProcessCrashed(xiiStringView message)
{
  ShutdownProcess();
  m_Status = xiiStatus(message);
  xiiLogEntryDelegate logger([this](xiiLogEntry& ref_entry) { m_LogEntries.PushBack(std::move(ref_entry)); });
  xiiLog::Error(&logger, message);
  xiiLog::Error(&xiiAssetProcessor::GetSingleton()->m_CuratorLog, message);
}

bool xiiProcessTask::IsConnected()
{
  return m_pIPC->IsConnected();
}

bool xiiProcessTask::HasProcessCrashed()
{
  return m_pIPC->IsClientAlive();
}

bool xiiProcessTask::Tick(bool bStartNewWork)
{
  while (true)
  {
    switch (m_State)
    {
      case State::LookingForWork:
      {
        if (!bStartNewWork)
          return false; // don't call later

        m_LogEntries.Clear();
        m_TransitiveHull.Clear();
        m_Status = xiiStatus(XII_SUCCESS);
        {
          XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);

          if (!GetNextAssetToProcess(m_AssetGuid, m_AssetPath))
          {
            m_AssetGuid = xiiUuid();
            m_AssetPath.Clear();

            if (m_pIPC->IsClientAlive() && m_pIPC->IsConnected())
            {
              // If we have nothing else to do, we might as well free some resource memory the process holds.
              xiiFreeAllResourcesMsg msg;
              m_pIPC->SendMessage(&msg);
            }

            return bStartNewWork; // call again if we should be looking for new work
          }

          xiiAssetInfo::TransformState state = xiiAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash, m_uiPackageHash);
          XII_ASSERT_DEV(state == xiiAssetInfo::TransformState::NeedsTransform || state == xiiAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");

          xiiSet<xiiString> dependencies;
          xiiStringBuilder  sTemp;
          xiiAssetCurator::GetSingleton()->GenerateTransitiveHull(xiiConversionUtils::ToString(m_AssetGuid, sTemp), dependencies, true, true);

          m_TransitiveHull.Reserve(dependencies.GetCount());
          for (const xiiString& str : dependencies)
          {
            m_TransitiveHull.PushBack(str);
          }
        }

        if (!m_pIPC->IsClientAlive() || !m_pIPC->IsConnected())
        {
          if (StartProcess().Failed())
          {
            m_State = State::ReportResult;
            OnProcessCrashed("Asset processor did not launch");
          }
          else
          {
            m_State = State::WaitingForConnection;
            return true; // call again later
          }
        }
        else
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::WaitingForConnection:
      {
        if (!m_pIPC->IsClientAlive())
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed while waiting for connection");
          break;
        }

        if (m_pIPC->IsConnected())
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::Ready:
      {
        xiiLog::Info(&xiiAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());
        // Send and wait
        xiiProcessAssetMsg msg;
        msg.m_AssetGuid   = m_AssetGuid;
        msg.m_AssetHash   = m_uiAssetHash;
        msg.m_ThumbHash   = m_uiThumbHash;
        msg.m_PackageHash = m_uiPackageHash;
        msg.m_sAssetPath  = m_AssetPath;
        msg.m_DepRefHull.Swap(m_TransitiveHull);
        msg.m_sPlatform = xiiAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

        if (m_pIPC->SendMessage(&msg))
        {
          m_State = State::Processing;
          return true; // call again later
        }
        else
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed, failed to send message");
        }
      }
      break;
      case State::Processing:
      {
        m_pIPC->ProcessMessages();
        if (!m_pIPC->IsClientAlive())
        {
          OnProcessCrashed("Asset Processor crashed during processing");
          m_State = State::ReportResult;
        }
      }
      break;
      case State::ReportResult:
      {
        if (m_Status.Succeeded())
        {
          xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          xiiAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
        }
        else
        {
          if (m_Status.m_Result == xiiTransformResult::NeedsImport)
          {
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, xiiAssetInfo::TransformState::NeedsImport);
          }
          else
          {
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, xiiAssetInfo::TransformState::TransformError);
          }
        }

        {
          XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
          xiiAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
        }

        m_State = State::LookingForWork;
      }
      break;
    }
  }
}

xiiUInt32 xiiProcessThread::Run()
{
  xiiAssetProcessor::GetSingleton()->Run();
  return 0;
}
