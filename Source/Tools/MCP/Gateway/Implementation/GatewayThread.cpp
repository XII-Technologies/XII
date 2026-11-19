/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <MCP/Gateway/GatewayThread.h>

xiiMCPGatewayThread::xiiMCPGatewayThread(const xiiMCPGatewayConfiguration& configuration) :
  xiiThread("MCP Gateway", 1024 * 1024),
  m_Configuration(configuration)
{
  Start();
}

xiiMCPGatewayThread::~xiiMCPGatewayThread()
{
  m_bStop = true;
  CancelRequests();
  Join();
}

bool xiiMCPGatewayThread::Submit(xiiStringView sRequest)
{
  XII_LOCK(m_Mutex);
  if (m_Requests.GetCount() >= 16 || m_bStop || m_pPendingConfiguration || m_bApplying)
  {
    return false;
  }
  m_Requests.PushBack(xiiString(sRequest));
  return true;
}

bool xiiMCPGatewayThread::ApplyConfiguration(const xiiMCPGatewayConfiguration& configuration)
{
  XII_LOCK(m_Mutex);
  if (m_bStop || m_pPendingConfiguration || m_bApplying || !m_Requests.IsEmpty() || !m_TargetChanges.IsEmpty())
  {
    return false;
  }
  m_pPendingConfiguration = XII_DEFAULT_NEW(xiiMCPGatewayConfiguration, configuration);
  return true;
}

bool xiiMCPGatewayThread::SetTargetEnabled(xiiStringView sEndpoint, bool bEnabled)
{
  XII_LOCK(m_Mutex);
  if (m_TargetChanges.GetCount() >= 16 || m_bStop || m_pPendingConfiguration || m_bApplying)
  {
    return false;
  }
  m_TargetChanges.PushBack(TargetChange{xiiString(sEndpoint), bEnabled});
  return true;
}

void xiiMCPGatewayThread::CancelRequests()
{
  XII_LOCK(m_Mutex);
  m_Requests.Clear();
  m_bCanceled = true;
}

void xiiMCPGatewayThread::Read(xiiVariantDictionary& out_snapshot, xiiDeque<xiiString>& out_replies)
{
  XII_LOCK(m_Mutex);
  out_snapshot = m_Snapshot;
  out_replies.Swap(m_Replies);
}

xiiUInt32 xiiMCPGatewayThread::Run()
{
  xiiUniquePtr<xiiMCPGateway> pGateway = XII_DEFAULT_NEW(xiiMCPGateway);
  auto                        status   = pGateway->Start(m_Configuration);
  xiiString                   sConfigurationError;
  xiiString                   sTargetStatus;
  if (status.Failed())
  {
    sConfigurationError = status.GetMessageString();
    pGateway.Clear();
  }

  xiiMCPSession session;
  session.m_State = xiiMCPSession::State::Ready;
  xiiTime   nextSnapshot;
  xiiUInt64 uiConfigurationRevision = 0;
  while (!m_bStop)
  {
    TargetChange                             targetChange;
    xiiString                                sRequest;
    xiiUniquePtr<xiiMCPGatewayConfiguration> pPending;
    {
      XII_LOCK(m_Mutex);
      pPending    = std::move(m_pPendingConfiguration);
      m_bApplying = pPending != nullptr;
      if (!m_Requests.IsEmpty())
      {
        m_bCanceled = false;
        sRequest    = std::move(m_Requests.PeekFront());
        m_Requests.PopFront();
      }
      if (!m_TargetChanges.IsEmpty())
      {
        targetChange = std::move(m_TargetChanges.PeekFront());
        m_TargetChanges.PopFront();
      }
    }
    if (pPending)
    {
      const bool bCanRestore = pGateway != nullptr;
      pGateway.Clear();
      pGateway = XII_DEFAULT_NEW(xiiMCPGateway);
      status   = pGateway->Start(*pPending);
      if (status.Succeeded())
      {
        m_Configuration = std::move(*pPending);
      }
      else
      {
        pGateway.Clear();
        if (bCanRestore)
        {
          pGateway = XII_DEFAULT_NEW(xiiMCPGateway);
          if (pGateway->Start(m_Configuration).Failed())
          {
            pGateway.Clear();
          }
        }
      }
      sConfigurationError = status.Succeeded() ? xiiString() : status.GetMessageString();
      ++uiConfigurationRevision;
      nextSnapshot = {};
      XII_LOCK(m_Mutex);
      m_bApplying = false;
    }
    if (pGateway)
    {
      if (!targetChange.m_sEndpoint.IsEmpty())
      {
        const auto targetStatus = pGateway->SetTargetEnabled(targetChange.m_sEndpoint, targetChange.m_bEnabled);
        if (targetStatus.Failed())
        {
          sTargetStatus = targetStatus.GetMessageString();
        }
        else
        {
          sTargetStatus = targetChange.m_bEnabled ? "Connection requested." : "Target disconnected.";
        }
      }
      pGateway->Update();
    }
    if (!sRequest.IsEmpty())
    {
      xiiStringBuilder sReply;
      if (pGateway)
      {
        pGateway->Process(sRequest, session, sReply, &m_bCanceled);
      }
      else
      {
        sReply = "The gateway is unavailable.";
      }
      XII_LOCK(m_Mutex);
      if (m_Replies.GetCount() == 16)
      {
        m_Replies.PopFront();
      }
      m_Replies.PushBack(xiiString(sReply));
    }

    const auto now = xiiTime::Now();
    if (now >= nextSnapshot)
    {
      auto snapshot                     = pGateway ? pGateway->GetSnapshot() : xiiVariantDictionary();
      snapshot["error"]                 = sConfigurationError;
      snapshot["targetStatus"]          = sTargetStatus;
      snapshot["configurationRevision"] = uiConfigurationRevision;
      XII_LOCK(m_Mutex);
      m_Snapshot   = std::move(snapshot);
      nextSnapshot = now + xiiTime::MakeFromMilliseconds(200);
    }
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  }
  return 0;
}
