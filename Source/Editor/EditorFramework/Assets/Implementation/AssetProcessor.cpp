/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Utilities/ConversionUtils.h>
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
  StartProcessTask(true);
}

void xiiAssetProcessor::StartProcessTask(bool bAutomatic)
{
  if (!xiiToolsProject::IsProjectOpen() || !xiiAssetCurator::GetSingleton()->IsReady())
  {
    xiiLog::Warning("Asset processing requires a ready project and completed asset indexing.");
    return;
  }
  XII_LOCK(m_ProcessorMutex);
  if (bAutomatic)
    m_bAutomatic = true;
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

  m_bForceStop          = false;
  m_uiProjectGeneration = xiiToolsProject::GetProjectGeneration();
  m_ProcessTaskState    = ProcessTaskState::Running;

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

void xiiAssetProcessor::StopProcessTask(bool bForce, bool bWait)
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
        if (!bForce && !bWait)
          return;
        break;
      default:
      case ProcessTaskState::Stopped:
        if (!bWait)
          return;
        break;
    }
  }

  {
    XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
    m_TransformProgress.m_bCancelRequested = true;
    BroadcastTransformProgress();
  }

  if (bForce)
    m_bForceStop = true;
  if ((bForce || bWait) && m_pThread)
  {
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
  while (m_ProcessTaskState == ProcessTaskState::Running && xiiToolsProject::IsProjectGenerationCurrent(m_uiProjectGeneration))
  {
    for (xiiUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      m_ProcessTasks[i].Tick(true);
    }
    if (!m_bAutomatic)
    {
      XII_LOCK(m_ProcessorMutex);
      XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
      if (m_RequestedAssets.IsEmpty())
      {
        m_ProcessTaskState = ProcessTaskState::Stopping;
        break;
      }
    }
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
  }

  {
    XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
    m_TransformProgress.m_bCancelRequested = !m_RequestedAssets.IsEmpty();
    m_TransformProgress.m_uiCanceled += m_TransformRequests.GetCount();
    m_TransformProgress.m_uiCanceled += m_FinalTransformRequests.GetCount();
    for (const auto& request : m_TransformRequests)
      if (--m_RequestedAssets[request.m_AssetGuid] == 0)
        m_RequestedAssets.Remove(request.m_AssetGuid);
    m_TransformRequests.Clear();
    for (const auto& request : m_FinalTransformRequests)
      if (--m_RequestedAssets[request.m_AssetGuid] == 0)
        m_RequestedAssets.Remove(request.m_AssetGuid);
    m_FinalTransformRequests.Clear();
    BroadcastTransformProgress();
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

  {
    XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
    BroadcastTransformProgress(true);
  }

  XII_LOCK(m_ProcessorMutex);
  m_ProcessTasks.Clear();
  m_ProcessTaskState = ProcessTaskState::Stopped;
  m_bForceStop       = false;
  m_bAutomatic       = false;
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
    m_LogEntries.Swap(pMsg->m_LogEntries);
    if (m_Status.m_Result == xiiTransformResult::NeedsImport && !m_Request.m_bImport)
    {
      // Keep the same reservation and aggregate request while the isolated child
      // computes the changes that used to trigger a modal main-thread import.
      m_Request.m_bImport = true;
      m_State             = State::WaitingForImport;
    }
    else if (m_Request.m_bImport && m_Status.Succeeded())
    {
      XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
      m_sImportBefore  = pMsg->m_sImportBefore;
      m_sImportAfter   = pMsg->m_sImportAfter;
      m_bImportPending = true;
      m_bImportApplied = false;
      m_State          = State::ApplyingImport;
    }
    else
      m_State = State::ReportResult;
  }
}

bool xiiProcessTask::GetNextAssetToProcess(xiiAssetInfo* pInfo, xiiUuid& out_guid, xiiDataDirPath& out_path)
{
  if (xiiAssetProcessor::GetSingleton()->m_DeferredImports.Contains(pInfo->m_Info->m_DocumentID))
    return false;
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
        if (xiiAssetCurator::GetSingleton()->m_Updating.Contains(pFileInfo->m_Info->m_DocumentID))
        {
          bComplete = false;
          continue;
        }
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
          case xiiAssetInfo::TransformState::NeedsImport:
          {
            bComplete = false;
            return pFileInfo;
          }
          case xiiAssetInfo::TransformState::UpToDate:
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

  if (bComplete && !xiiAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) && !xiiAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
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

  auto* processor = xiiAssetProcessor::GetSingleton();
  auto* curator   = xiiAssetCurator::GetSingleton();
  m_bRequested    = false;
  if (!curator->IsReady() || !xiiToolsProject::IsProjectOpen() || !xiiToolsProject::IsProjectGenerationCurrent(processor->m_uiProjectGeneration))
    return false;
  if (curator->IsCheckingFileSystem())
    return false; // Keep reservations for running work; dispatch new work after the rescan.
  // Manager-wide outputs are rebuilt after ordinary assets and their in-flight writes.
  if (processor->m_TransformRequests.IsEmpty() && curator->m_Updating.IsEmpty() && !processor->m_FinalTransformRequests.IsEmpty())
  {
    processor->m_TransformRequests.PushBack(std::move(processor->m_FinalTransformRequests.PeekFront()));
    processor->m_FinalTransformRequests.PopFront();
  }
  for (xiiUInt32 i = 0; i < processor->m_TransformRequests.GetCount(); ++i)
  {
    auto&              request = processor->m_TransformRequests[i];
    xiiProcessAssetMsg fresh;
    if (xiiAssetProcessor::PrepareAssetRequest(request.m_AssetGuid, fresh, request.m_sPlatform).Succeeded())
    {
      fresh.m_sPlatform      = request.m_sPlatform;
      fresh.m_TransformFlags = request.m_TransformFlags;
      fresh.m_bImport        = request.m_bImport;
      request                = std::move(fresh);
    }
    xiiDynamicArray<xiiUuid> reserved;
    reserved.PushBack(request.m_AssetGuid);
    for (const auto& dependency : request.m_DepRefHull)
    {
      auto asset = curator->FindSubAsset(dependency);
      if (asset.isValid() && !reserved.Contains(asset->m_pAssetInfo->m_Info->m_DocumentID))
        reserved.PushBack(asset->m_pAssetInfo->m_Info->m_DocumentID);
    }
    bool bBusy = false;
    for (const auto& guid : reserved)
      bBusy |= curator->m_Updating.Contains(guid);
    if (bBusy)
      continue;

    m_Request = request;
    processor->m_TransformRequests.RemoveAtAndCopy(i);
    m_ReservedAssets = std::move(reserved);
    for (const auto& guid : m_ReservedAssets)
      curator->m_Updating.Insert(guid);
    m_bRequested = true;
    out_guid     = m_Request.m_AssetGuid;
    out_path     = curator->GetAssetInfo(out_guid) ? curator->GetAssetInfo(out_guid)->m_Path : xiiDataDirPath();
    return true;
  }
  if (!processor->m_bAutomatic || !processor->m_FinalTransformRequests.IsEmpty())
    return false;

  for (auto state : {xiiAssetInfo::NeedsImport, xiiAssetInfo::NeedsTransform, xiiAssetInfo::NeedsThumbnail})
  {
    for (auto it = curator->m_TransformState[state].GetIterator(); it.IsValid(); ++it)
    {
      if (auto* info = curator->GetAssetInfo(it.Key()); info && GetNextAssetToProcess(info, out_guid, out_path))
        return true;
    }
  }

  return false;
}

void xiiProcessTask::OnProcessCrashed(xiiStringView message)
{
  ShutdownProcess();
  m_Status = xiiStatus(message);
  if (xiiAssetProcessor::GetSingleton()->m_bForceStop)
    return; // Deliberate termination is cancellation, not a failed asset diagnostic.
  xiiLogEntryDelegate logger([this](xiiLogEntry& ref_entry) { m_LogEntries.PushBack(std::move(ref_entry)); });
  xiiLog::Error(&logger, message);
  xiiLog::Error(&xiiAssetProcessor::GetSingleton()->m_CuratorLog, message);
}

bool xiiProcessTask::Tick(bool bStartNewWork)
{
  while (true)
  {
    switch (m_State)
    {
      case State::LookingForWork:
      {
        if (!bStartNewWork || xiiAssetProcessor::GetSingleton()->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Running)
          return false; // don't call later

        m_LogEntries.Clear();
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

          if (!m_bRequested && xiiAssetProcessor::PrepareAssetRequest(m_AssetGuid, m_Request).Failed())
          {
            m_Status = xiiTransformStatus("Asset disappeared while preparing its background processing request.");
            m_State  = State::ReportResult;
            break;
          }
          if (m_bRequested && !m_Request.m_TransformFlags.IsSet(xiiTransformFlags::ForceTransform))
          {
            auto*           curator = xiiAssetCurator::GetSingleton();
            xiiUInt64       hash = 0, thumbnail = 0, package = 0;
            const xiiString profileName(m_Request.m_sPlatform);
            const auto      index = curator->FindAssetProfileByName(profileName);
            if (index != xiiInvalidIndex && curator->IsAssetUpToDate(m_AssetGuid, curator->GetAssetProfile(index), nullptr, hash, thumbnail, package) == xiiAssetInfo::UpToDate)
            {
              m_State = State::ReportResult;
              break;
            }
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
        else
          return true;
      }
      break;
      case State::Ready:
      {
        xiiLog::Info(&xiiAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());
        if (m_pIPC->SendMessage(&m_Request))
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
        else if (m_State == State::Processing)
          return true;
      }
      break;
      case State::WaitingForImport:
      {
        auto* processor = xiiAssetProcessor::GetSingleton();
        if (processor->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Running)
        {
          m_State = State::ReportResult;
          break;
        }
        // Source imports can discover new sidecar assets outside the known
        // dependency hull. Serialize these passes to avoid competing creators.
        for (const auto& task : processor->m_ProcessTasks)
          if (&task != this && task.m_Request.m_bImport &&
              (task.m_State == State::Ready || task.m_State == State::Processing || task.m_State == State::ApplyingImport))
            return true;
        m_State = State::Ready;
      }
      break;
      case State::ApplyingImport:
      {
        XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
        if (xiiAssetProcessor::GetSingleton()->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Running)
        {
          // A GUI-thread join cannot wait for its own next tick. Discard an
          // unapplied graph on stop; the child never saved the source document.
          m_bImportPending = false;
          m_Status         = xiiTransformStatus(xiiTransformResult::NeedsImport);
        }
        else if (!m_bImportApplied)
          return true;
        m_sImportBefore.Clear();
        m_sImportAfter.Clear();
        m_State = State::ReportResult;
      }
      break;
      case State::ReportResult:
      {
        XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
        const bool bCanceled = !xiiToolsProject::IsProjectGenerationCurrent(xiiAssetProcessor::GetSingleton()->m_uiProjectGeneration) || xiiAssetProcessor::GetSingleton()->m_bForceStop ||
          (m_Request.m_bImport && m_Status.m_Result == xiiTransformResult::NeedsImport && xiiAssetProcessor::GetSingleton()->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Running);
        xiiProcessAssetMsg current;
        const bool         bChanged = m_bRequested && !bCanceled && xiiAssetProcessor::PrepareAssetRequest(m_AssetGuid, current, m_Request.m_sPlatform).Succeeded() &&
          (current.m_AssetHash != m_Request.m_AssetHash || current.m_ThumbHash != m_Request.m_ThumbHash || current.m_PackageHash != m_Request.m_PackageHash);
        if (m_Status.Succeeded() && !bCanceled && !bChanged && !m_Request.m_bImport)
        {
          xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          xiiAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
        }
        else if (m_Status.Failed() && !bCanceled && !bChanged)
        {
          if (m_Status.m_Result == xiiTransformResult::NeedsImport)
          {
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, xiiAssetInfo::TransformState::NeedsImport);
          }
          else
          {
            if (m_LogEntries.IsEmpty() && !m_Status.m_sMessage.IsEmpty())
            {
              auto& entry  = m_LogEntries.ExpandAndGetRef();
              entry.m_Type = xiiLogMsgType::ErrorMsg;
              entry.m_sMsg = m_Status.m_sMessage;
            }
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
            xiiAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, xiiAssetInfo::TransformState::TransformError);
          }
        }

        {
          XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
          xiiAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
          for (const auto& guid : m_ReservedAssets)
          {
            xiiAssetCurator::GetSingleton()->m_Updating.Remove(guid);
            if (guid != m_AssetGuid)
              xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(guid);
          }
          if (bCanceled || bChanged)
            xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          m_ReservedAssets.Clear();
          if (m_bRequested)
          {
            auto* processor = xiiAssetProcessor::GetSingleton();
            if (--processor->m_RequestedAssets[m_AssetGuid] == 0)
              processor->m_RequestedAssets.Remove(m_AssetGuid);
            auto& progress = processor->m_TransformProgress;
            if (bCanceled)
              ++progress.m_uiCanceled;
            else
            {
              ++progress.m_uiCompleted;
              progress.m_uiFailed += !bChanged && m_Status.m_Result == xiiTransformResult::Failure ? 1 : 0;
              progress.m_uiNeedsImport += !bChanged && m_Status.m_Result == xiiTransformResult::NeedsImport ? 1 : 0;
            }
            if ((bChanged || (m_Request.m_bImport && m_Status.Succeeded())) && processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running)
            {
              current.m_TransformFlags = m_Request.m_TransformFlags;
              if (m_Request.m_bImport && m_Status.Succeeded())
                current.m_TransformFlags.Remove(xiiTransformFlags::ForceTransform);
              current.m_sPlatform = m_Request.m_sPlatform;
              processor->m_TransformRequests.PushBack(std::move(current));
              ++processor->m_RequestedAssets[m_AssetGuid];
              ++progress.m_uiTotal;
            }
            processor->BroadcastTransformProgress(processor->m_RequestedAssets.IsEmpty());
          }
        }

        xiiAssetProcessorEvent result;
        result.m_Type       = xiiAssetProcessorEvent::Type::AssetProcessed;
        result.m_Result     = m_Status.m_Result;
        result.m_sAssetPath = m_AssetPath.GetDataDirRelativePath();
        result.m_bRequested = m_bRequested;
        result.m_bCanceled  = bCanceled;
        xiiAssetProcessor::GetSingleton()->m_Events.Broadcast(result);

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
