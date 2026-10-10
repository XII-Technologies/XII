/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

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
  explicit xiiSynchronizedGraph(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Graph(pAllocator)
  {
  }

  /// Read access returns a locked object that unlocks on destruction. The graph is immutable for const access.
  xiiLockedObject<xiiMutex, const xiiGraph> Read() const
  {
    return xiiLockedObject<xiiMutex, const xiiGraph>(m_Mutex, &m_Graph);
  }

  /// Write access returns a locked object that unlocks on destruction. The graph is mutable even for const access.
  xiiLockedObject<xiiMutex, xiiGraph> Write()
  {
    return xiiLockedObject<xiiMutex, xiiGraph>(m_Mutex, &m_Graph);
  }

private:
  mutable xiiMutex m_Mutex;
  xiiGraph         m_Graph;
};
