/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkEndpoint.h>
#include <Foundation/Network/NetworkError.h>

XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
#  include <sys/socket.h>
#endif

namespace xiiNetworkInternal
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  using NativeSocket                         = SOCKET;
  using NativeLength                         = int;
  constexpr NativeSocket InvalidNativeSocket = INVALID_SOCKET;

  /// One Winsock startup reference per live socket or scoped resolver operation.
  class RuntimeLease
  {
  public:
    RuntimeLease();
    ~RuntimeLease();
    RuntimeLease(const RuntimeLease&)            = delete;
    RuntimeLease& operator=(const RuntimeLease&) = delete;
    RuntimeLease(RuntimeLease&&)                 = delete;
    RuntimeLease&   operator=(RuntimeLease&&)    = delete;
    xiiNetworkError GetError() const { return m_Error; }
    void            Detach() { m_bAcquired = false; }

  private:
    xiiNetworkError m_Error;
    bool            m_bAcquired = false;
  };
#else
  using NativeSocket                         = int;
  using NativeLength                         = socklen_t;
  constexpr NativeSocket InvalidNativeSocket = -1;
  class RuntimeLease
  {
  public:
    xiiNetworkError GetError() const { return {}; }
    void            Detach() const {}
  };
#endif

  struct NativeAddress
  {
    sockaddr_storage m_Storage  = {};
    NativeLength     m_uiLength = sizeof(sockaddr_storage);
    sockaddr*        Get() { return reinterpret_cast<sockaddr*>(&m_Storage); }
    const sockaddr*  Get() const { return reinterpret_cast<const sockaddr*>(&m_Storage); }
  };
  xiiNetworkError ToNative(const xiiNetworkEndpoint& endpoint, NativeAddress& out_native);
  xiiNetworkError FromNative(const sockaddr* pNative, NativeLength uiLength, xiiNetworkEndpoint& out_endpoint);

  xiiNetworkError TranslateSocketError(int iNativeCode);
  int             GetLastSocketError();
  int             CloseNative(NativeSocket hSocket);
  void            ReleaseRuntime();
  int             SetOption(NativeSocket hSocket, int iLevel, int iName, const void* pValue, NativeLength uiSize);
  int             GetOption(NativeSocket hSocket, int iLevel, int iName, void* out_pValue, NativeLength* inout_pSize);
  xiiNetworkError SetNonBlockingNative(NativeSocket hSocket, bool bNonBlocking);
  NativeSocket    CreateNative(int iFamily, int iType, bool bNonBlocking, xiiNetworkError& out_error);
  NativeSocket    AcceptNative(NativeSocket hListener, sockaddr* out_pAddress, NativeLength* inout_pLength, bool bNonBlocking, xiiNetworkError& out_error);
  xiiNetworkError WaitNative(NativeSocket hSocket, bool bRead, int iMilliseconds);
  xiiNetworkError SetTimeoutNative(NativeSocket hSocket, int iOption, xiiUInt32 uiMilliseconds);
} // namespace xiiNetworkInternal
