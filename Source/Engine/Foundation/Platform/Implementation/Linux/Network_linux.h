/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

namespace xiiNetworkInternal
{
  NativeSocket CreateNative(int iFamily, int iType, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const int hSocket = ::socket(iFamily, iType | SOCK_CLOEXEC | (bNonBlocking ? SOCK_NONBLOCK : 0), 0);
    if (hSocket < 0)
    {
      out_error = TranslateSocketError(errno);
    }
    return hSocket;
  }

  NativeSocket AcceptNative(NativeSocket hListener, sockaddr* out_pAddress, NativeLength* inout_pLength, bool bNonBlocking, xiiNetworkError& out_error)
  {
    const int hSocket = ::accept4(hListener, out_pAddress, inout_pLength, SOCK_CLOEXEC | (bNonBlocking ? SOCK_NONBLOCK : 0));
    if (hSocket < 0)
    {
      out_error = TranslateSocketError(errno);
    }
    return hSocket;
  }
} // namespace xiiNetworkInternal
