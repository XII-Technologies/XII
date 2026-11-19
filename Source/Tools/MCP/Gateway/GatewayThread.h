/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <MCP/Gateway.h>

/// Runs all gateway and protocol state on one worker. The UI exchanges bounded queues and snapshots.
class xiiMCPGatewayThread final : public xiiThread
{
public:
  explicit xiiMCPGatewayThread(const xiiMCPGatewayConfiguration& configuration);
  ~xiiMCPGatewayThread() override;

  bool Submit(xiiStringView sRequest);
  bool SetTargetEnabled(xiiStringView sEndpoint, bool bEnabled);
  bool ApplyConfiguration(const xiiMCPGatewayConfiguration& configuration);
  void CancelRequests();
  void Read(xiiVariantDictionary& out_snapshot, xiiDeque<xiiString>& out_replies);

private:
  struct TargetChange
  {
    xiiString m_sEndpoint;
    bool      m_bEnabled = false;
  };

  xiiUInt32 Run() override;

  xiiMCPGatewayConfiguration               m_Configuration;
  xiiMutex                                 m_Mutex;
  xiiVariantDictionary                     m_Snapshot;
  xiiDeque<xiiString>                      m_Requests;
  xiiDeque<xiiString>                      m_Replies;
  xiiDeque<TargetChange>                   m_TargetChanges;
  xiiUniquePtr<xiiMCPGatewayConfiguration> m_pPendingConfiguration;
  xiiAtomicBool                            m_bStop     = false;
  xiiAtomicBool                            m_bCanceled = false;
  bool                                     m_bApplying = false;
};
