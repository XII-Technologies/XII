/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_OSX)

#  include <Foundation/Platform/Implementation/NetworkPlatform.h>

#  include <cerrno>
#  include <fcntl.h>
#  include <unistd.h>

namespace xiiNetworkInternal
{

  xiiNetworkError ConfigureDescriptor(NativeSocket hSocket, bool bNonBlocking)
  {
    // BSD has no accept4/SOCK_CLOEXEC. Applications must serialize fork/exec with descriptor creation.
    const int iDescriptorFlags = ::fcntl(hSocket, F_GETFD, 0);
    if (iDescriptorFlags < 0 || ::fcntl(hSocket, F_SETFD, iDescriptorFlags | FD_CLOEXEC) < 0)
    {
      return TranslateSocketError(errno);
    }
    const int iEnabled = 1;
    if (::setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, &iEnabled, sizeof(iEnabled)) != 0)
    {
      return TranslateSocketError(errno);
    }
    return SetNonBlockingNative(hSocket, bNonBlocking);
  }

  NativeSocket CreateNative(int iFamily, int iType, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const int hSocket = ::socket(iFamily, iType, 0);
    if (hSocket < 0)
    {
      out_error = TranslateSocketError(errno);
      return -1;
    }
    out_error = ConfigureDescriptor(hSocket, bNonBlocking);
    if (out_error.Failed())
    {
      ::close(hSocket);
      return -1;
    }
    return hSocket;
  }

  NativeSocket AcceptNative(NativeSocket hListener, sockaddr* out_pAddress, NativeLength* inout_pLength, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const int hSocket = ::accept(hListener, out_pAddress, inout_pLength);
    if (hSocket < 0)
    {
      out_error = TranslateSocketError(errno);
      return -1;
    }
    out_error = ConfigureDescriptor(hSocket, bNonBlocking);
    if (out_error.Failed())
    {
      ::close(hSocket);
      return -1;
    }
    return hSocket;
  }

} // namespace xiiNetworkInternal

#endif
