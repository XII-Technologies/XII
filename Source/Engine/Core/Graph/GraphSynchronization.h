/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Core/Graph/Graph.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>

/// Optional scoped synchronization using Foundation's mutex and locked-object facility.
/// Both reads and writes are exclusive. Keep views/cursors inside the access scope; handles may escape it.
/// Operations and algorithms run atomically with respect to other access scopes. Lock ordering across graphs
/// remains the caller's responsibility. The underlying plain graph carries no synchronization overhead.
class xiiSynchronizedGraph
{
public:
  explicit xiiSynchronizedGraph(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) : m_Graph(pAllocator) {}
  xiiLockedObject<xiiMutex, const xiiGraph> Read() const { return xiiLockedObject<xiiMutex, const xiiGraph>(m_Mutex, &m_Graph); }
  xiiLockedObject<xiiMutex, xiiGraph>       Write() { return xiiLockedObject<xiiMutex, xiiGraph>(m_Mutex, &m_Graph); }

private:
  mutable xiiMutex m_Mutex;
  xiiGraph         m_Graph;
};
