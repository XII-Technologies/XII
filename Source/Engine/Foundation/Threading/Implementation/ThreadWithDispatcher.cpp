/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadWithDispatcher.h>

xiiThreadWithDispatcher::xiiThreadWithDispatcher(xiiStringView sName /*= "xiiThreadWithDispatcher"*/, xiiUInt32 uiStackSize /*= 128 * 1024*/) :
  xiiThread(sName, uiStackSize)
{
}

xiiThreadWithDispatcher::~xiiThreadWithDispatcher() = default;

void xiiThreadWithDispatcher::Dispatch(DispatchFunction&& delegate)
{
  XII_LOCK(m_QueueMutex);
  m_ActiveQueue.PushBack(std::move(delegate));
}

void xiiThreadWithDispatcher::DispatchQueue()
{
  {
    XII_LOCK(m_QueueMutex);
    std::swap(m_ActiveQueue, m_CurrentlyBeingDispatchedQueue);
  }

  for (const auto& pDelegate : m_CurrentlyBeingDispatchedQueue)
  {
    pDelegate();
  }

  m_CurrentlyBeingDispatchedQueue.Clear();
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadWithDispatcher);
