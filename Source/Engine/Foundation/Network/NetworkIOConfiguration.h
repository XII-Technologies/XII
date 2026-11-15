/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkBufferPool.h>
#include <Foundation/Time/Time.h>

/// Limits reserve capacity before accepting work. No accepted completion is discarded under pressure.
struct XII_FOUNDATION_DLL xiiNetworkIOConfiguration
{
  xiiUInt32 m_uiWorkerThreadCount          = 2;
  xiiUInt32 m_uiResolverThreadCount        = 2;
  xiiUInt32 m_uiMaxRegisteredSockets       = 4096;
  xiiUInt32 m_uiMaxOutstandingOperations   = 8192;
  xiiUInt32 m_uiMaxOperationsPerSocket     = 128;
  xiiUInt32 m_uiMaxQueuedCompletions       = 8192;
  xiiUInt32 m_uiMaxQueuedSubmissions       = 8192;
  xiiUInt32 m_uiMaxReceiveRequests         = 4096;
  xiiUInt32 m_uiMaxResolverRequests        = 64;
  xiiUInt32 m_uiMaxAcceptOperations        = 256;
  xiiUInt32 m_uiAcceptConcurrencyPerSocket = 8;
  xiiUInt32 m_uiNativeEventBatchSize       = 64;
  xiiUInt32 m_uiSocketsPerIteration        = 64;
  xiiUInt32 m_uiAttemptsPerSocket          = 16;
  xiiUInt32 m_uiMaxTransferBytesPerAttempt = 65536;
  xiiUInt64 m_uiMaxQueuedSendBytes         = 64ULL * 1024 * 1024;
  /// Optional monotonic processing quantum per socket. Zero uses only attempt and byte budgets.
  xiiTime m_SocketProcessingBudget = xiiTime::MakeFromMilliseconds(1);
  /// -1 means infinite, otherwise a finite nonnegative duration.
  xiiTime                           m_DefaultTimeout   = xiiTime::MakeFromSeconds(-1);
  bool                              m_bEnableProfiling = false;
  xiiNetworkBufferPoolConfiguration m_BufferPool;
};
