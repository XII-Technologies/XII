/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Platform/Implementation/NetworkPlatform.h>

#  include <arpa/inet.h>
#  include <cerrno>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <poll.h>
#  include <sys/time.h>
#  include <sys/uio.h>
#  include <unistd.h>

namespace xiiNetworkInternal
{
  xiiNetworkError TranslateSocketError(int iNativeCode)
  {
    xiiNetworkErrorCode code = xiiNetworkErrorCode::Unknown;
    switch (iNativeCode)
    {
      case 0:
        return {};
      case EINVAL:
        code = xiiNetworkErrorCode::InvalidArgument;
        break;
      case EAFNOSUPPORT:
        code = xiiNetworkErrorCode::UnsupportedAddressFamily;
        break;
      case EPROTONOSUPPORT:
        code = xiiNetworkErrorCode::UnsupportedAddressFamily;
        break;
      case EADDRINUSE:
        code = xiiNetworkErrorCode::AddressInUse;
        break;
      case EADDRNOTAVAIL:
        code = xiiNetworkErrorCode::AddressNotAvailable;
        break;
      case EACCES:
        code = xiiNetworkErrorCode::PermissionDenied;
        break;
      case EPERM:
        code = xiiNetworkErrorCode::PermissionDenied;
        break;
      case ECONNREFUSED:
        code = xiiNetworkErrorCode::ConnectionRefused;
        break;
      case ECONNRESET:
        code = xiiNetworkErrorCode::ConnectionReset;
        break;
      case ECONNABORTED:
        code = xiiNetworkErrorCode::ConnectionAborted;
        break;
      case EPIPE:
        code = xiiNetworkErrorCode::ConnectionClosed;
        break;
      case ENETRESET:
        code = xiiNetworkErrorCode::ConnectionReset;
        break;
      case ENETUNREACH:
        code = xiiNetworkErrorCode::NetworkUnreachable;
        break;
      case ENETDOWN:
        code = xiiNetworkErrorCode::NetworkUnreachable;
        break;
      case EHOSTUNREACH:
        code = xiiNetworkErrorCode::HostUnreachable;
        break;
      case ETIMEDOUT:
        code = xiiNetworkErrorCode::TimedOut;
        break;
      case EINPROGRESS:
        code = xiiNetworkErrorCode::InProgress;
        break;
      case EALREADY:
        code = xiiNetworkErrorCode::InProgress;
        break;
      case EAGAIN:
        code = xiiNetworkErrorCode::WouldBlock;
        break;
      case EINTR:
        code = xiiNetworkErrorCode::Interrupted;
        break;
      case ENOMEM:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case ENOBUFS:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case EMFILE:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case ENFILE:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case EMSGSIZE:
        code = xiiNetworkErrorCode::MessageTooLarge;
        break;
      case EOPNOTSUPP:
        code = xiiNetworkErrorCode::UnsupportedOperation;
        break;
      case ENOPROTOOPT:
        code = xiiNetworkErrorCode::UnsupportedOperation;
        break;
      case EBADF:
        code = xiiNetworkErrorCode::NotOpen;
        break;
      case ENOTSOCK:
        code = xiiNetworkErrorCode::NotOpen;
        break;
      case ENOTCONN:
        code = xiiNetworkErrorCode::NotConnected;
        break;
      case EISCONN:
        code = xiiNetworkErrorCode::AlreadyConnected;
        break;
#  if EAGAIN != EWOULDBLOCK
      case EWOULDBLOCK:
        code = xiiNetworkErrorCode::WouldBlock;
        break;
#  endif
      default:
        break;
    }
    return xiiNetworkError(code, iNativeCode, xiiNetworkErrorDomain::Socket);
  }

  int  GetLastError() { return errno; }
  int  CloseNative(NativeSocket hSocket) { return ::close(hSocket); }
  void ReleaseRuntime() {}



  int SetOption(NativeSocket hSocket, int iLevel, int iName, const void* pValue, NativeLength uiSize)
  {
    return ::setsockopt(hSocket, iLevel, iName, pValue, uiSize);
  }
  int GetOption(NativeSocket hSocket, int iLevel, int iName, void* out_pValue, NativeLength* inout_pSize)
  {
    return ::getsockopt(hSocket, iLevel, iName, out_pValue, inout_pSize);
  }

  xiiNetworkError SetNonBlockingNative(NativeSocket hSocket, bool bNonBlocking)
  {
    const int iFlags = ::fcntl(hSocket, F_GETFL, 0);
    if (iFlags < 0 || ::fcntl(hSocket, F_SETFL, bNonBlocking ? (iFlags | O_NONBLOCK) : (iFlags & ~O_NONBLOCK)) < 0)
    {
      return TranslateSocketError(errno);
    }
    return {};
  }

  xiiNetworkError WaitNative(NativeSocket hSocket, bool bRead, int iMilliseconds)
  {
    pollfd descriptor = {};
    descriptor.fd     = hSocket;
    descriptor.events = bRead ? POLLIN : POLLOUT;
    const int iResult = ::poll(&descriptor, 1, iMilliseconds);
    if (iResult < 0)
    {
      return TranslateSocketError(errno);
    }
    if (iResult == 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::TimedOut);
    }
    if ((descriptor.revents & POLLNVAL) != 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
    }
    return {};
  }

  xiiNetworkError SetTimeoutNative(NativeSocket hSocket, int iOption, xiiUInt32 uiMilliseconds)
  {
    timeval value = {};
    value.tv_sec  = static_cast<decltype(value.tv_sec)>(uiMilliseconds / 1000);
    value.tv_usec = static_cast<decltype(value.tv_usec)>((uiMilliseconds % 1000) * 1000);
    return ::setsockopt(hSocket, SOL_SOCKET, iOption, &value, sizeof(value)) == 0 ? xiiNetworkError() : TranslateSocketError(errno);
  }

} // namespace xiiNetworkInternal

#endif
