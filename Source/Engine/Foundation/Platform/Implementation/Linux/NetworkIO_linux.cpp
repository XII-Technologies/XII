/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Network/Implementation/NetworkIOInternal.h>
#  include <Foundation/Platform/Implementation/NetworkPlatform.h>
#  include <Foundation/Platform/Implementation/NetworkSocketAccess.h>

#  include <cerrno>
#  include <sys/epoll.h>
#  include <sys/eventfd.h>
#  include <unistd.h>

#  include <ctime>

XII_DEFINE_AS_POD_TYPE(epoll_event);

namespace
{
  struct NetworkEpollState
  {
    NetworkEpollState()                                         = default;
    NetworkEpollState(const NetworkEpollState&)                 = delete;
    NetworkEpollState& operator=(const NetworkEpollState&)      = delete;
    NetworkEpollState(NetworkEpollState&&)                      = delete;
    NetworkEpollState&           operator=(NetworkEpollState&&) = delete;
    int                          m_hEpoll                       = -1;
    int                          m_hWake                        = -1;
    xiiDynamicArray<epoll_event> m_Events;
    ~NetworkEpollState()
    {
      if (m_hWake >= 0)
      {
        ::close(m_hWake);
      }
      if (m_hEpoll >= 0)
      {
        ::close(m_hEpoll);
      }
    }
  };
} // namespace

namespace xiiNetworkIOInternal
{
  xiiTime CurrentThreadCpuTime()
  {
    timespec time = {};
    if (::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time) != 0)
    {
      return {};
    }
    return xiiTime::MakeFromSeconds(static_cast<double>(time.tv_sec) + (static_cast<double>(time.tv_nsec) / 1000000000.0));
  }

  xiiNetworkError CreateWorker(Worker& worker)
  {
    xiiUniquePtr<NetworkEpollState> pState = XII_DEFAULT_NEW(NetworkEpollState);
    pState->m_Events.SetCount(worker.m_Events.GetCount());
    pState->m_hEpoll = ::epoll_create1(EPOLL_CLOEXEC);
    if (pState->m_hEpoll < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    pState->m_hWake = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (pState->m_hWake < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    epoll_event event = {};
    event.events      = EPOLLIN;
    event.data.u64    = 0;
    if (::epoll_ctl(pState->m_hEpoll, EPOLL_CTL_ADD, pState->m_hWake, &event) != 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    worker.m_Statistics.m_uiInternalMemoryBytes += sizeof(NetworkEpollState) + pState->m_Events.GetHeapMemoryUsage();
    worker.m_pNativeData = pState.Release();
    return {};
  }

  void DestroyWorker(Worker& worker)
  {
    auto* pState = static_cast<NetworkEpollState*>(worker.m_pNativeData);
    if (pState != nullptr)
    {
      worker.m_Statistics.m_uiInternalMemoryBytes -= sizeof(NetworkEpollState) + pState->m_Events.GetHeapMemoryUsage();
    }
    XII_DEFAULT_DELETE(pState);
    worker.m_pNativeData = nullptr;
  }

  void Wake(Worker& worker)
  {
    if (worker.m_pNativeData == nullptr)
    {
      return;
    }
    auto&          state   = *static_cast<NetworkEpollState*>(worker.m_pNativeData);
    const uint64_t uiOne   = 1;
    ssize_t        iResult = 0;
    do
    {
      iResult = ::write(state.m_hWake, &uiOne, sizeof(uiOne));
    } while (iResult < 0 && errno == EINTR);
    XII_ASSERT_ALWAYS(iResult == sizeof(uiOne) || (iResult < 0 && errno == EAGAIN), "An owned eventfd must remain writable or already signaled.");
  }

  xiiNetworkError Wait(Worker& worker, xiiInt32 iMilliseconds, xiiUInt32& out_uiCount)
  {
    auto&     state  = *static_cast<NetworkEpollState*>(worker.m_pNativeData);
    const int iCount = ::epoll_wait(state.m_hEpoll, state.m_Events.GetData(), static_cast<int>(state.m_Events.GetCount()), iMilliseconds);
    out_uiCount      = 0;
    if (iCount < 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    for (int iIndex = 0; iIndex < iCount; ++iIndex)
    {
      if (state.m_Events[iIndex].data.u64 == 0)
      {
        uint64_t uiValue = 0;
        while (::read(state.m_hWake, &uiValue, sizeof(uiValue)) < 0 && errno == EINTR)
        {
        }
        continue;
      }
      NativeEvent& event = worker.m_Events[out_uiCount++];
      event              = {};
      event.m_uiToken    = state.m_Events[iIndex].data.u64;
      // ERR/HUP/RDHUP trigger the same nonblocking operation path, which retrieves precise EOF/errors.
    }
    return {};
  }

  void Remove(Worker& worker, Registration& registration)
  {
    if (registration.m_bNativeRegistered)
    {
      auto& state = *static_cast<NetworkEpollState*>(worker.m_pNativeData);
      ::epoll_ctl(state.m_hEpoll, EPOLL_CTL_DEL, static_cast<int>(registration.GetNativeHandle()), nullptr);
      registration.m_bNativeRegistered = false;
      registration.m_uiNativeInterest  = 0;
    }
  }

  xiiNetworkError Arm(Worker& worker, Registration& registration)
  {
    const xiiUInt32 uiInterest = (registration.m_Read.m_pHead != nullptr ? EPOLLIN : 0U) |
      (registration.m_Write.m_pHead != nullptr ? EPOLLOUT : 0U);
    if (uiInterest == 0)
    {
      Remove(worker, registration);
      return {};
    }
    auto&       state    = *static_cast<NetworkEpollState*>(worker.m_pNativeData);
    epoll_event event    = {};
    event.events         = uiInterest | EPOLLONESHOT | EPOLLRDHUP;
    event.data.u64       = registration.m_uiNativeToken;
    const int iOperation = registration.m_bNativeRegistered ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    if (::epoll_ctl(state.m_hEpoll, iOperation, static_cast<int>(registration.GetNativeHandle()), &event) != 0)
    {
      return xiiNetworkInternal::TranslateSocketError(errno);
    }
    registration.m_bNativeRegistered = true;
    registration.m_uiNativeInterest  = uiInterest;
    return {};
  }
} // namespace xiiNetworkIOInternal

#endif
