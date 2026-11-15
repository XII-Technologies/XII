/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Network/Implementation/NetworkIOInternal.h>
#  include <Foundation/Platform/Implementation/NetworkPlatform.h>
#  include <Foundation/Platform/Implementation/NetworkSocketAccess.h>

#  include <cerrno>
#  include <fcntl.h>
#  include <sys/event.h>
#  include <unistd.h>

#  include <cstdint>

XII_DEFINE_AS_POD_TYPE(struct kevent);

namespace
{
  struct NetworkKqueueState
  {
    int                            m_hQueue = -1;
    xiiDynamicArray<struct kevent> m_Events;
    ~NetworkKqueueState()
    {
      if (m_hQueue >= 0)
      {
        ::close(m_hQueue);
      }
    }
  };
} // namespace

namespace xiiNetworkIOInternal
{
  xiiNetworkError CreateWorker(Worker& worker)
  {
    static_assert(sizeof(uintptr_t) >= sizeof(xiiUInt64), "macOS registration tokens require its supported 64-bit ABI.");
    xiiUniquePtr<NetworkKqueueState> pState = XII_DEFAULT_NEW(NetworkKqueueState);
    pState->m_Events.SetCount(worker.m_Events.GetCount());
    pState->m_hQueue = ::kqueue();
    if (pState->m_hQueue < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    const int iFlags = ::fcntl(pState->m_hQueue, F_GETFD);
    if (iFlags < 0 || ::fcntl(pState->m_hQueue, F_SETFD, iFlags | FD_CLOEXEC) < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    struct kevent event = {};
    EV_SET(&event, 0, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(pState->m_hQueue, &event, 1, nullptr, 0, nullptr) != 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    worker.m_Statistics.m_uiInternalMemoryBytes += sizeof(NetworkKqueueState) + pState->m_Events.GetHeapMemoryUsage();
    worker.m_pNativeData = pState.Release();
    return {};
  }

  void DestroyWorker(Worker& worker)
  {
    auto* pState = static_cast<NetworkKqueueState*>(worker.m_pNativeData);
    XII_DEFAULT_DELETE(pState);
    worker.m_pNativeData = nullptr;
  }

  void Wake(Worker& worker)
  {
    if (worker.m_pNativeData == nullptr)
    {
      return;
    }
    auto&         state = *static_cast<NetworkKqueueState*>(worker.m_pNativeData);
    struct kevent event = {};
    EV_SET(&event, 0, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    int iResult;
    do
    {
      iResult = ::kevent(state.m_hQueue, &event, 1, nullptr, 0, nullptr);
    } while (iResult < 0 && errno == EINTR);
    XII_ASSERT_ALWAYS(iResult == 0, "The owned kqueue user event must remain available until workers join.");
  }

  xiiNetworkError Wait(Worker& worker, xiiInt32 iMilliseconds, xiiUInt32& out_uiCount)
  {
    auto&    state   = *static_cast<NetworkKqueueState*>(worker.m_pNativeData);
    timespec timeout = {};
    if (iMilliseconds >= 0)
    {
      timeout.tv_sec  = iMilliseconds / 1000;
      timeout.tv_nsec = (iMilliseconds % 1000) * 1000000;
    }
    const int iCount = ::kevent(state.m_hQueue, nullptr, 0, state.m_Events.GetData(), static_cast<int>(state.m_Events.GetCount()), iMilliseconds < 0 ? nullptr : &timeout);
    out_uiCount      = 0;
    if (iCount < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    for (int iIndex = 0; iIndex < iCount; ++iIndex)
    {
      const struct kevent& native = state.m_Events[iIndex];
      if (native.filter == EVFILT_USER)
      {
        continue;
      }
      if ((native.flags & EV_ERROR) != 0 && native.data != 0)
      {
        return xiiNetworkInternal::TranslateSocketError(static_cast<int>(native.data));
      }
      NativeEvent& event = worker.m_Events[out_uiCount++];
      event              = {};
      event.m_uiToken    = static_cast<xiiUInt64>(reinterpret_cast<uintptr_t>(native.udata));
      // EV_EOF still permits draining readable bytes before the stream reports EOF/reset.
    }
    return {};
  }

  void Remove(Worker& worker, Registration& registration)
  {
    if (!registration.m_bNativeRegistered)
    {
      return;
    }
    auto&           state    = *static_cast<NetworkKqueueState*>(worker.m_pNativeData);
    const uintptr_t uiSocket = static_cast<uintptr_t>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
    for (xiiUInt32 uiBit = 1; uiBit <= 2; uiBit <<= 1)
    {
      if ((registration.m_uiNativeInterest & uiBit) != 0)
      {
        struct kevent event = {};
        EV_SET(&event, uiSocket, uiBit == 1 ? EVFILT_READ : EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        ::kevent(state.m_hQueue, &event, 1, nullptr, 0, nullptr);
      }
    }
    registration.m_bNativeRegistered = false;
    registration.m_uiNativeInterest  = 0;
  }

  xiiNetworkError Arm(Worker& worker, Registration& registration)
  {
    auto&           state      = *static_cast<NetworkKqueueState*>(worker.m_pNativeData);
    const xiiUInt32 uiInterest = (registration.m_Read.m_pHead != nullptr ? 1U : 0U) | (registration.m_Write.m_pHead != nullptr ? 2U : 0U);
    const uintptr_t uiSocket   = static_cast<uintptr_t>(xiiNetworkSocketAccess::BorrowNativeHandle(registration.GetSocket()));
    for (xiiUInt32 uiBit = 1; uiBit <= 2; uiBit <<= 1)
    {
      struct kevent event = {};
      if ((uiInterest & uiBit) != 0)
      {
        EV_SET(&event, uiSocket, uiBit == 1 ? EVFILT_READ : EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_DISPATCH, 0, 0,
               reinterpret_cast<void*>(static_cast<uintptr_t>(registration.m_uiNativeToken)));
      }
      else if ((registration.m_uiNativeInterest & uiBit) != 0)
      {
        EV_SET(&event, uiSocket, uiBit == 1 ? EVFILT_READ : EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
      }
      else
      {
        continue;
      }
      if (::kevent(state.m_hQueue, &event, 1, nullptr, 0, nullptr) != 0)
      {
        return xiiNetworkInternal::TranslateSocketError(errno);
      }
    }
    registration.m_uiNativeInterest  = uiInterest;
    registration.m_bNativeRegistered = uiInterest != 0;
    return {};
  }
} // namespace xiiNetworkIOInternal

#endif
