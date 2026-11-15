/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Implementation/NetworkPlatform.h>

#  include <iphlpapi.h>
#  include <mstcpip.h>

namespace xiiNetworkInternal
{
  xiiNetworkError TranslateSocketError(int iNativeCode)
  {
    xiiNetworkErrorCode code = xiiNetworkErrorCode::Unknown;
    switch (iNativeCode)
    {
      case 0:
        return {};
      case WSAEINVAL:
        code = xiiNetworkErrorCode::InvalidArgument;
        break;
      case WSAEAFNOSUPPORT:
        code = xiiNetworkErrorCode::UnsupportedAddressFamily;
        break;
      case WSAEPROTONOSUPPORT:
        code = xiiNetworkErrorCode::UnsupportedAddressFamily;
        break;
      case WSAEADDRINUSE:
        code = xiiNetworkErrorCode::AddressInUse;
        break;
      case WSAEADDRNOTAVAIL:
        code = xiiNetworkErrorCode::AddressNotAvailable;
        break;
      case WSAEACCES:
        code = xiiNetworkErrorCode::PermissionDenied;
        break;
      case WSAECONNREFUSED:
        code = xiiNetworkErrorCode::ConnectionRefused;
        break;
      case WSAECONNRESET:
        code = xiiNetworkErrorCode::ConnectionReset;
        break;
      case WSAECONNABORTED:
        code = xiiNetworkErrorCode::ConnectionAborted;
        break;
      case WSAESHUTDOWN:
        code = xiiNetworkErrorCode::ConnectionClosed;
        break;
      case WSAENETRESET:
        code = xiiNetworkErrorCode::ConnectionReset;
        break;
      case WSAENETUNREACH:
        code = xiiNetworkErrorCode::NetworkUnreachable;
        break;
      case WSAENETDOWN:
        code = xiiNetworkErrorCode::NetworkUnreachable;
        break;
      case WSAEHOSTUNREACH:
        code = xiiNetworkErrorCode::HostUnreachable;
        break;
      case WSAETIMEDOUT:
        code = xiiNetworkErrorCode::TimedOut;
        break;
      case WSAEINPROGRESS:
        code = xiiNetworkErrorCode::InProgress;
        break;
      case WSAEALREADY:
        code = xiiNetworkErrorCode::InProgress;
        break;
      case WSAEWOULDBLOCK:
        code = xiiNetworkErrorCode::WouldBlock;
        break;
      case WSAEINTR:
        code = xiiNetworkErrorCode::Interrupted;
        break;
      case WSAENOBUFS:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case WSAEMFILE:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case WSAEMSGSIZE:
        code = xiiNetworkErrorCode::MessageTooLarge;
        break;
      case WSAEOPNOTSUPP:
        code = xiiNetworkErrorCode::UnsupportedOperation;
        break;
      case WSAENOPROTOOPT:
        code = xiiNetworkErrorCode::UnsupportedOperation;
        break;
      case WSAENOTSOCK:
        code = xiiNetworkErrorCode::NotOpen;
        break;
      case WSAENOTCONN:
        code = xiiNetworkErrorCode::NotConnected;
        break;
      case WSAEISCONN:
        code = xiiNetworkErrorCode::AlreadyConnected;
        break;
      case WSANOTINITIALISED:
        code = xiiNetworkErrorCode::InitializationFailed;
        break;
      default:
        break;
    }
    return xiiNetworkError(code, iNativeCode, xiiNetworkErrorDomain::Socket);
  }

  int  GetLastError() { return ::WSAGetLastError(); }
  int  CloseNative(NativeSocket hSocket) { return ::closesocket(hSocket); }
  void ReleaseRuntime() { ::WSACleanup(); }


  int SetOption(NativeSocket hSocket, int iLevel, int iName, const void* pValue, NativeLength iSize)
  {
    return ::setsockopt(hSocket, iLevel, iName, static_cast<const char*>(pValue), iSize);
  }
  int GetOption(NativeSocket hSocket, int iLevel, int iName, void* out_pValue, NativeLength* inout_pSize)
  {
    return ::getsockopt(hSocket, iLevel, iName, static_cast<char*>(out_pValue), inout_pSize);
  }

  xiiNetworkError SetNonBlockingNative(NativeSocket hSocket, bool bNonBlocking)
  {
    u_long uiMode = bNonBlocking ? 1UL : 0UL;
    return ::ioctlsocket(hSocket, FIONBIO, &uiMode) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
  }

  NativeSocket CreateNative(int iFamily, int iType, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const SOCKET hSocket = ::WSASocketW(iFamily, iType, 0, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
    if (hSocket == INVALID_SOCKET)
    {
      out_error = TranslateSocketError(GetLastError());
      return INVALID_SOCKET;
    }
    out_error = SetNonBlockingNative(hSocket, bNonBlocking);
    if (out_error.Failed())
    {
      ::closesocket(hSocket);
      return INVALID_SOCKET;
    }
    return hSocket;
  }

  NativeSocket AcceptNative(NativeSocket hListener, sockaddr* out_pAddress, NativeLength* inout_pLength, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const SOCKET hSocket = ::accept(hListener, out_pAddress, inout_pLength);
    if (hSocket == INVALID_SOCKET)
    {
      out_error = TranslateSocketError(GetLastError());
      return INVALID_SOCKET;
    }
    // accept inherits the listener's no-inherit property, but enforce it before publication as well.
    if (!::SetHandleInformation(reinterpret_cast<HANDLE>(hSocket), HANDLE_FLAG_INHERIT, 0))
    {
      out_error = xiiNetworkError(xiiNetworkErrorCode::Unknown, static_cast<xiiInt32>(::GetLastError()), xiiNetworkErrorDomain::Initialization);
    }
    else
    {
      out_error = SetNonBlockingNative(hSocket, bNonBlocking);
    }
    if (out_error.Failed())
    {
      ::closesocket(hSocket);
      return INVALID_SOCKET;
    }
    return hSocket;
  }

  xiiNetworkError WaitNative(NativeSocket hSocket, bool bRead, int iMilliseconds)
  {
    // select includes exceptfds for failed nonblocking connects on all supported Windows versions.
    fd_set ready;
    fd_set errors;
    FD_ZERO(&ready);
    FD_ZERO(&errors);
    FD_SET(hSocket, &ready);
    FD_SET(hSocket, &errors);
    timeval timeout   = {};
    timeout.tv_sec    = iMilliseconds / 1000;
    timeout.tv_usec   = (iMilliseconds % 1000) * 1000;
    const int iResult = ::select(0, bRead ? &ready : nullptr, bRead ? nullptr : &ready, &errors, &timeout);
    if (iResult < 0)
    {
      return TranslateSocketError(GetLastError());
    }
    return iResult == 0 ? xiiNetworkError(xiiNetworkErrorCode::TimedOut) : xiiNetworkError();
  }

  xiiNetworkError SetTimeoutNative(NativeSocket hSocket, int iOption, xiiUInt32 uiMilliseconds)
  {
    const DWORD uiValue = uiMilliseconds;
    return SetOption(hSocket, SOL_SOCKET, iOption, &uiValue, sizeof(uiValue)) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
  }

  /// One successful WSAStartup reference per live socket or resolver operation. Winsock serializes
  /// its process reference count; no engine global mutex or subsystem shutdown ordering is required.
  /// Ownership of the startup reference transfers to a socket only after complete initialization.


  RuntimeLease::RuntimeLease()
  {
    WSADATA   data    = {};
    const int iResult = ::WSAStartup(MAKEWORD(2, 2), &data);
    if (iResult != 0)
    {
      m_Error = xiiNetworkError(xiiNetworkErrorCode::InitializationFailed, iResult, xiiNetworkErrorDomain::Initialization);
    }
    else if (data.wVersion != MAKEWORD(2, 2))
    {
      ::WSACleanup();
      m_Error = xiiNetworkError(xiiNetworkErrorCode::InitializationFailed, WSAVERNOTSUPPORTED, xiiNetworkErrorDomain::Initialization);
    }
    else
    {
      m_bAcquired = true;
    }
  }
  RuntimeLease::~RuntimeLease()
  {
    if (m_bAcquired)
    {
      ::WSACleanup();
    }
  }

} // namespace xiiNetworkInternal

#endif
