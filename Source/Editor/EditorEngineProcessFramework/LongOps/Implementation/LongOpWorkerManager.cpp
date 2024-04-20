#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

XII_IMPLEMENT_SINGLETON(xiiLongOpWorkerManager);

xiiLongOpWorkerManager::xiiLongOpWorkerManager() :
  m_SingletonRegistrar(this)
{
}

xiiLongOpWorkerManager::~xiiLongOpWorkerManager() = default;

class xiiLongOpTask final : public xiiTask
{
public:
  xiiLongOpWorker* m_pWorkerOp = nullptr;
  xiiUuid          m_OperationGuid;
  xiiProgress*     m_pProgress = nullptr;

  xiiLongOpTask()
  {
    xiiStringBuilder name;
    name.SetFormat("Long Op: '{}'", "TODO: NAME"); // TODO
    ConfigureTask(name, xiiTaskNesting::Maybe);
  }

  ~xiiLongOpTask() = default;

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    xiiDataBuffer                                         resultData;
    xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> storage(&resultData);
    xiiMemoryStreamWriter                                 writer(&storage);

    const xiiResult res = m_pWorkerOp->Execute(*m_pProgress, writer);

    xiiLongOpWorkerManager::GetSingleton()->WorkerOperationFinished(m_OperationGuid, res, std::move(resultData));
  }
};

void xiiLongOpWorkerManager::ProcessCommunicationChannelEventHandler(const xiiProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = xiiDynamicCast<const xiiLongOpReplicationMsg*>(e.m_pMessage))
  {
    XII_LOCK(m_Mutex);

    xiiRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const xiiRTTI*           pRtti = xiiRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfoPtr = m_WorkerOps.ExpandAndGetRef();
    opInfoPtr       = XII_DEFAULT_NEW(WorkerOpInfo);

    auto& opInfo           = *opInfoPtr;
    opInfo.m_DocumentGuid  = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pWorkerOp     = pRtti->GetAllocator()->Allocate<xiiLongOpWorker>();

    LaunchWorkerOperation(opInfo, reader);
    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiLongOpResultMsg*>(e.m_pMessage))
  {
    XII_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      XII_ASSERT_DEBUG(pMsg->m_bSuccess == false, "Only Cancel messages are allowed to send to the processor");

      pOpInfo->m_Progress.UserClickedCancel();
    }

    return;
  }
}

void xiiLongOpWorkerManager::LaunchWorkerOperation(WorkerOpInfo& opInfo, xiiStreamReader& config)
{
  opInfo.m_Progress.SetCompletion(0.0f);
  opInfo.m_Progress.m_pUserData = &opInfo;
  opInfo.m_Progress.m_Events.AddEventHandler(
    xiiMakeDelegate(&xiiLongOpWorkerManager::WorkerProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  SendProgress(opInfo);

  if (opInfo.m_pWorkerOp->InitializeExecution(config, opInfo.m_DocumentGuid).Failed())
  {
    WorkerOperationFinished(opInfo.m_OperationGuid, XII_FAILURE, xiiDataBuffer());
  }
  else
  {
    xiiSharedPtr<xiiLongOpTask> pTask = XII_DEFAULT_NEW(xiiLongOpTask);
    pTask->m_OperationGuid            = opInfo.m_OperationGuid;
    pTask->m_pWorkerOp                = opInfo.m_pWorkerOp.Borrow();
    pTask->m_pProgress                = &opInfo.m_Progress;
    opInfo.m_TaskID                   = xiiTaskSystem::StartSingleTask(pTask, xiiTaskPriority::LongRunning);
  }
}

void xiiLongOpWorkerManager::WorkerOperationFinished(xiiUuid operationGuid, xiiResult result, xiiDataBuffer&& resultData)
{
  XII_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(operationGuid);

  if (pOpInfo == nullptr)
    return;

  // tell the controller about the result
  {
    xiiLongOpResultMsg msg;
    msg.m_OperationGuid = operationGuid;
    msg.m_bSuccess      = result.Succeeded();
    msg.m_ResultData    = std::move(resultData);

    m_pCommunicationChannel->SendMessage(&msg);
  }

  RemoveOperation(operationGuid);
}

void xiiLongOpWorkerManager::WorkerProgressBarEventHandler(const xiiProgressEvent& e)
{
  if (e.m_Type == xiiProgressEvent::Type::ProgressChanged)
  {
    auto pOpInfo = static_cast<WorkerOpInfo*>(e.m_pProgressbar->m_pUserData);

    SendProgress(*pOpInfo);
  }
}

void xiiLongOpWorkerManager::RemoveOperation(xiiUuid opGuid)
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < m_WorkerOps.GetCount(); ++i)
  {
    if (m_WorkerOps[i]->m_OperationGuid == opGuid)
    {
      m_WorkerOps.RemoveAtAndSwap(i);
      return;
    }
  }
}

xiiLongOpWorkerManager::WorkerOpInfo* xiiLongOpWorkerManager::GetOperation(const xiiUuid& opGuid) const
{
  XII_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_WorkerOps)
  {
    if (opInfoPtr->m_OperationGuid == opGuid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void xiiLongOpWorkerManager::SendProgress(WorkerOpInfo& opInfo)
{
  xiiLongOpProgressMsg msg;
  msg.m_OperationGuid = opInfo.m_OperationGuid;
  msg.m_fCompletion   = opInfo.m_Progress.GetCompletion();

  m_pCommunicationChannel->SendMessage(&msg);
}
