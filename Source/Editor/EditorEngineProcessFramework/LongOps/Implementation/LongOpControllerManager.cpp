#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

XII_IMPLEMENT_SINGLETON(xiiLongOpControllerManager);

xiiLongOpControllerManager::xiiLongOpControllerManager() :
  m_SingletonRegistrar(this)
{
}

xiiLongOpControllerManager::~xiiLongOpControllerManager() = default;

void xiiLongOpControllerManager::ProcessCommunicationChannelEventHandler(const xiiProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = xiiDynamicCast<const xiiLongOpProgressMsg*>(e.m_pMessage))
  {
    XII_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_fCompletion = pMsg->m_fCompletion;

      BroadcastProgress(*pOpInfo);
    }

    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiLongOpResultMsg*>(e.m_pMessage))
  {
    XII_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_bIsRunning      = false;
      pOpInfo->m_StartOrDuration = xiiTime::Now() - pOpInfo->m_StartOrDuration;
      pOpInfo->m_fCompletion     = 0.0f;

      pOpInfo->m_pProxyOp->Finalize(pMsg->m_bSuccess ? XII_SUCCESS : XII_FAILURE, pMsg->m_ResultData);

      // TODO: show success/failure in UI
      BroadcastProgress(*pOpInfo);
    }
  }
}

void xiiLongOpControllerManager::StartOperation(xiiUuid opGuid)
{
  XII_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || pOpInfo->m_bIsRunning)
    return;

  pOpInfo->m_StartOrDuration = xiiTime::Now();
  pOpInfo->m_bIsRunning      = true;

  ReplicateToWorkerProcess(*pOpInfo);

  BroadcastProgress(*pOpInfo);
}

void xiiLongOpControllerManager::CancelOperation(xiiUuid opGuid)
{
  XII_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || !pOpInfo->m_bIsRunning)
    return;

  // send a cancel message to the processor
  xiiLongOpResultMsg msg;
  msg.m_OperationGuid = opGuid;
  msg.m_bSuccess      = false;
  m_pCommunicationChannel->SendMessage(&msg);
}

void xiiLongOpControllerManager::RemoveOperation(xiiUuid opGuid)
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    if (m_ProxyOps[i]->m_OperationGuid == opGuid)
    {
      m_ProxyOps.RemoveAtAndCopy(i);

      // broadcast the removal to the UI
      {
        xiiLongOpControllerEvent e;
        e.m_Type          = xiiLongOpControllerEvent::Type::OpRemoved;
        e.m_OperationGuid = opGuid;

        m_Events.Broadcast(e);
      }

      return;
    }
  }
}

void xiiLongOpControllerManager::RegisterLongOp(const xiiUuid& documentGuid, const xiiUuid& componentGuid, const char* szLongOpType)
{
  const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(szLongOpType);
  if (pRtti == nullptr)
  {
    xiiLog::Error("Can't register long op of unknown type '{}'", szLongOpType);
    return;
  }

  auto& opInfoPtr = m_ProxyOps.ExpandAndGetRef();
  opInfoPtr       = XII_DEFAULT_NEW(ProxyOpInfo);

  auto& opInfo           = *opInfoPtr;
  opInfo.m_DocumentGuid  = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_OperationGuid = xiiUuid::CreateUuid();

  opInfo.m_pProxyOp = pRtti->GetAllocator()->Allocate<xiiLongOpProxy>();
  opInfo.m_pProxyOp->InitializeRegistered(documentGuid, componentGuid);

  xiiLongOpControllerEvent e;
  e.m_Type          = xiiLongOpControllerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void xiiLongOpControllerManager::UnregisterLongOp(const xiiUuid& documentGuid, const xiiUuid& componentGuid, const char* szLongOpType)
{
  for (xiiUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    auto& opInfoPtr = m_ProxyOps[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_DocumentGuid == documentGuid &&
        opInfoPtr->m_pProxyOp->GetDynamicRTTI()->GetTypeName() == szLongOpType)
    {
      RemoveOperation(opInfoPtr->m_OperationGuid);
      return;
    }
  }
}

xiiLongOpControllerManager::ProxyOpInfo* xiiLongOpControllerManager::GetOperation(const xiiUuid& opGuid)
{
  XII_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_ProxyOps)
  {
    if (opInfoPtr->m_OperationGuid == opGuid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void xiiLongOpControllerManager::CancelAndRemoveAllOpsForDocument(const xiiUuid& documentGuid)
{
  {
    XII_LOCK(m_Mutex);

    for (auto& opInfoPtr : m_ProxyOps)
    {
      CancelOperation(opInfoPtr->m_OperationGuid);
    }
  }

  bool bOperationsStillActive = true;

  while (bOperationsStillActive)
  {
    bOperationsStillActive = false;
    m_pCommunicationChannel->ProcessMessages();

    {
      XII_LOCK(m_Mutex);

      for (xiiUInt32 i0 = m_ProxyOps.GetCount(); i0 > 0; --i0)
      {
        const xiiUInt32 i = i0 - 1;

        auto& op = m_ProxyOps[i];
        if (op->m_DocumentGuid == documentGuid)
        {
          if (op->m_bIsRunning)
          {
            bOperationsStillActive = true;
            break;
          }

          xiiLongOpControllerEvent e;
          e.m_Type          = xiiLongOpControllerEvent::Type::OpRemoved;
          e.m_OperationGuid = m_ProxyOps[i]->m_OperationGuid;

          m_ProxyOps.RemoveAtAndCopy(i);

          m_Events.Broadcast(e);
        }
      }
    }

    if (bOperationsStillActive)
    {
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(100));
    }
  }
}

void xiiLongOpControllerManager::ReplicateToWorkerProcess(ProxyOpInfo& opInfo)
{
  XII_LOCK(m_Mutex);

  // send the replication message
  {
    xiiLongOpReplicationMsg msg;

    xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> storage(&msg.m_ReplicationData);
    xiiMemoryStreamWriter                                 writer(&storage);

    xiiStringBuilder replType;
    opInfo.m_pProxyOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid     = opInfo.m_DocumentGuid;
    msg.m_OperationGuid    = opInfo.m_OperationGuid;

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

void xiiLongOpControllerManager::BroadcastProgress(ProxyOpInfo& opInfo)
{
  // as controller, broadcast progress to the UI
  xiiLongOpControllerEvent e;
  e.m_Type          = xiiLongOpControllerEvent::Type::OpProgress;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

#if 0
void xiiLongOpManager::AddLongOperation(xiiUniquePtr<xiiLongOp>&& pOperation, const xiiUuid& documentGuid)
{
  XII_LOCK(m_Mutex);

  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
  opInfoPtr       = XII_DEFAULT_NEW(LongOpInfo);

  auto& opInfo        = *opInfoPtr;
  opInfo.m_pOperation = std::move(pOperation);
  opInfo.m_OperationGuid.CreateNewUuid();
  opInfo.m_DocumentGuid         = documentGuid;
  opInfo.m_StartOrDuration      = xiiTime::Now();
  opInfo.m_Progress.m_pUserData = opInfo.m_pOperation.Borrow();
  opInfo.m_Progress.m_Events.AddEventHandler(
    xiiMakeDelegate(&xiiLongOpManager::ProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  xiiLongOp* pNewOp = opInfo.m_pOperation.Borrow();

  if (m_Mode == Mode::Processor || xiiDynamicCast<xiiLongOpProxy*>(pNewOp) != nullptr)
  {
    xiiStringBuilder replType;

    xiiLongOpReplicationMsg msg;

    xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> storage(&msg.m_ReplicationData);
    xiiMemoryStreamWriter                                 writer(&storage);

    pNewOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid     = opInfo.m_DocumentGuid;
    msg.m_OperationGuid    = opInfo.m_OperationGuid;
    msg.m_sDisplayName     = pNewOp->GetDisplayName();

    m_pCommunicationChannel->SendMessage(&msg);
  }

  LaunchWorkerOperation(opInfo);

  {
    xiiLongOpManagerEvent e;
    e.m_Type             = xiiLongOpManagerEvent::Type::OpAdded;
    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
    m_Events.Broadcast(e);
  }
}
#endif
