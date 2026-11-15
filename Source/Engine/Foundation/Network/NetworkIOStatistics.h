/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkBufferPool.h>
#include <Foundation/Time/Time.h>

/// Low-overhead context counters. Snapshots are thread-safe but not a transactional global snapshot.
struct XII_FOUNDATION_DLL xiiNetworkIOStatistics
{
  xiiUInt64                      m_uiRegisteredSockets    = 0;
  xiiUInt64                      m_uiRetainedOperations   = 0;
  xiiUInt64                      m_uiPendingOperations    = 0;
  xiiUInt64                      m_uiCompletedOperations  = 0;
  xiiUInt64                      m_uiCanceledOperations   = 0;
  xiiUInt64                      m_uiTimedOutOperations   = 0;
  xiiUInt64                      m_uiFailedOperations     = 0;
  xiiUInt64                      m_uiCompletionQueueDepth = 0;
  xiiUInt64                      m_uiSubmissionQueueDepth = 0;
  xiiUInt64                      m_uiBytesSent            = 0;
  xiiUInt64                      m_uiBytesReceived        = 0;
  xiiUInt64                      m_uiBackpressureEvents   = 0;
  xiiUInt64                      m_uiEventLoopWakeups     = 0;
  xiiUInt64                      m_uiNativeErrors         = 0;
  xiiUInt64                      m_uiInternalMemoryBytes  = 0;
  xiiTime                        m_WorkerActiveTime;
  xiiTime                        m_WorkerWaitTime;
  xiiTime                        m_TotalCompletionLatency;
  xiiTime                        m_MaxCompletionLatency;
  xiiNetworkBufferPoolStatistics m_BufferPool;
};
