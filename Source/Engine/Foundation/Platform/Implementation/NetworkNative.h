/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// Included only by NetworkPlatform.cpp after the selected native backend.
namespace xiiNetworkInternal
{
  NativeSocket Borrow(const xiiNetworkSocket& socket)
  {
    return static_cast<NativeSocket>(xiiNetworkSocketAccess::BorrowNativeHandle(socket));
  }

  int GetNativeFamily(xiiNetworkAddressFamily family)
  {
    switch (family)
    {
      case xiiNetworkAddressFamily::Ipv4:
        return AF_INET;
      case xiiNetworkAddressFamily::Ipv6:
        return AF_INET6;
      case xiiNetworkAddressFamily::Unspecified:
        return AF_UNSPEC;
    }
    return -1;
  }

  struct NativeAddress
  {
    sockaddr_storage m_Storage  = {};
    NativeLength     m_uiLength = sizeof(sockaddr_storage);
    sockaddr*        Get() { return reinterpret_cast<sockaddr*>(&m_Storage); }
    const sockaddr*  Get() const { return reinterpret_cast<const sockaddr*>(&m_Storage); }
  };

  xiiNetworkError ToNative(const xiiNetworkEndpoint& endpoint, NativeAddress& out_native)
  {
    out_native                       = {};
    const xiiNetworkAddress& address = endpoint.GetAddress();
    if (address.GetFamily() == xiiNetworkAddressFamily::Ipv4)
    {
      sockaddr_in native = {};
      native.sin_family  = AF_INET;
      native.sin_port    = htons(endpoint.GetPort());
      std::memcpy(&native.sin_addr, address.GetBytes().GetPtr(), 4);
      out_native.m_uiLength = sizeof(native);
      std::memcpy(&out_native.m_Storage, &native, sizeof(native));
    }
    else if (address.GetFamily() == xiiNetworkAddressFamily::Ipv6)
    {
      sockaddr_in6 native  = {};
      native.sin6_family   = AF_INET6;
      native.sin6_port     = htons(endpoint.GetPort());
      native.sin6_scope_id = address.GetScopeId();
      std::memcpy(&native.sin6_addr, address.GetBytes().GetPtr(), 16);
      out_native.m_uiLength = sizeof(native);
      std::memcpy(&out_native.m_Storage, &native, sizeof(native));
    }
    else
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
    }
#if XII_ENABLED(XII_PLATFORM_OSX)
    // BSD carries the native structure length in the common sockaddr prefix.
    out_native.Get()->sa_len = static_cast<xiiUInt8>(out_native.m_uiLength);
#endif
    return {};
  }

  xiiNetworkError FromNative(const sockaddr* pNative, NativeLength uiLength, xiiNetworkEndpoint& out_endpoint)
  {
    xiiNetworkAddress address;
    xiiUInt16         uiPort = 0;
    xiiNetworkError   error;
    if (pNative->sa_family == AF_INET && uiLength >= static_cast<NativeLength>(sizeof(sockaddr_in)))
    {
      sockaddr_in native;
      std::memcpy(&native, pNative, sizeof(native));
      error  = xiiNetworkAddress::FromBytes({reinterpret_cast<const xiiUInt8*>(&native.sin_addr), 4}, 0, address);
      uiPort = ntohs(native.sin_port);
    }
    else if (pNative->sa_family == AF_INET6 && uiLength >= static_cast<NativeLength>(sizeof(sockaddr_in6)))
    {
      sockaddr_in6 native;
      std::memcpy(&native, pNative, sizeof(native));
      error  = xiiNetworkAddress::FromBytes({reinterpret_cast<const xiiUInt8*>(&native.sin6_addr), 16}, native.sin6_scope_id, address);
      uiPort = ntohs(native.sin6_port);
    }
    else
    {
      return xiiNetworkError(xiiNetworkErrorCode::UnsupportedAddressFamily);
    }
    if (error.Failed())
    {
      return error;
    }
    out_endpoint = xiiNetworkEndpoint(address, uiPort);
    return {};
  }

  xiiNetworkError SetIntegerOption(const xiiNetworkSocket& socket, int iLevel, int iOption, int iValue)
  {
    if (!socket.IsOpen())
    {
      return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
    }
    return SetOption(Borrow(socket), iLevel, iOption, &iValue, sizeof(iValue)) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
  }

  xiiNetworkError Milliseconds(xiiTime time, xiiUInt32& out_uiMilliseconds)
  {
    const double fMilliseconds = std::ceil(time.GetMilliseconds());
    if (!std::isfinite(fMilliseconds) || time.IsNegative() || fMilliseconds > INT_MAX)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    out_uiMilliseconds = static_cast<xiiUInt32>(fMilliseconds);
    return {};
  }

  xiiNetworkError SetTimeout(const xiiNetworkSocket& socket, int iOption, xiiTime timeout)
  {
    if (!socket.IsOpen())
    {
      return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
    }
    xiiUInt32             uiMilliseconds;
    const xiiNetworkError error = Milliseconds(timeout, uiMilliseconds);
    if (error.Failed())
    {
      return error;
    }
    return SetTimeoutNative(Borrow(socket), iOption, uiMilliseconds);
  }

  xiiNetworkError TranslateResolverError(int iNativeCode, int iSystemError)
  {
    if (iNativeCode == 0)
    {
      return {};
    }
#ifdef EAI_SYSTEM
    if (iNativeCode == EAI_SYSTEM)
    {
      return TranslateSocketError(iSystemError);
    }
#else
    XII_IGNORE_UNUSED(iSystemError);
#endif
    xiiNetworkErrorCode code = xiiNetworkErrorCode::NameResolutionFailed;
    switch (iNativeCode)
    {
      case EAI_NONAME:
        code = xiiNetworkErrorCode::NameNotFound;
        break;
      case EAI_AGAIN:
        code = xiiNetworkErrorCode::WouldBlock;
        break;
      case EAI_MEMORY:
        code = xiiNetworkErrorCode::ResourceExhausted;
        break;
      case EAI_FAMILY:
        code = xiiNetworkErrorCode::UnsupportedAddressFamily;
        break;
      case EAI_SERVICE:
      case EAI_BADFLAGS:
        code = xiiNetworkErrorCode::InvalidArgument;
        break;
      default:
        break;
    }
    return xiiNetworkError(code, iNativeCode, xiiNetworkErrorDomain::Resolver);
  }

  bool ContainsNull(xiiStringView sText)
  {
    return !sText.IsEmpty() && std::memchr(sText.GetStartPointer(), 0, sText.GetElementCount()) != nullptr;
  }

  template <typename ByteType>
  xiiNetworkError ValidateBuffers(xiiArrayPtr<const xiiArrayPtr<ByteType>> buffers, bool bDatagram, xiiNetworkAddressFamily family, xiiUInt32& out_uiTotal)
  {
    out_uiTotal = 0;
    if (buffers.GetCount() > 64)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    for (const auto& bytes : buffers)
    {
      if (bytes.GetCount() > static_cast<xiiUInt32>(INT_MAX) - out_uiTotal)
      {
        return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
      }
      out_uiTotal += bytes.GetCount();
    }
    // IPv4's maximum UDP payload is smaller because of its IP header. No IPv6 jumbograms.
    if (bDatagram && out_uiTotal > (family == xiiNetworkAddressFamily::Ipv4 ? 65507U : 65527U))
    {
      return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
    }
    return {};
  }
} // namespace xiiNetworkInternal

xiiNetworkError xiiNetworkAddress::Parse(xiiStringView sText, xiiNetworkAddress& out_address)
{
  using namespace xiiNetworkInternal;
  char szLiteral[512];
  if (sText.IsEmpty() || sText.GetElementCount() >= sizeof(szLiteral) || ContainsNull(sText))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  std::memcpy(szLiteral, sText.GetStartPointer(), sText.GetElementCount());
  szLiteral[sText.GetElementCount()] = '\0';
  char*     pScope                   = std::strchr(szLiteral, '%');
  xiiUInt32 uiScopeId                = 0;
  if (pScope != nullptr)
  {
    *pScope++ = '\0';
    if (*pScope == '\0' || std::strchr(pScope, '%') != nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
    }
    bool bNumeric = true;
    for (const char* pCharacter = pScope; *pCharacter != '\0'; ++pCharacter)
    {
      if (*pCharacter < '0' || *pCharacter > '9')
      {
        bNumeric = false;
        break;
      }
    }
    if (bNumeric)
    {
      for (const char* pCharacter = pScope; *pCharacter != '\0'; ++pCharacter)
      {
        const xiiUInt32 uiDigit = static_cast<xiiUInt32>(*pCharacter - '0');
        if (uiScopeId > (0xFFFFFFFFU - uiDigit) / 10)
        {
          return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
        }
        uiScopeId = uiScopeId * 10 + uiDigit;
      }
    }
    else
    {
      uiScopeId = ::if_nametoindex(pScope);
      if (uiScopeId == 0)
      {
        return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
      }
    }
  }
  xiiUInt8 uiBytes[16] = {};
  if (pScope == nullptr && ::inet_pton(AF_INET, szLiteral, uiBytes) == 1)
  {
    return FromBytes({uiBytes, 4}, 0, out_address);
  }
  if (::inet_pton(AF_INET6, szLiteral, uiBytes) == 1)
  {
    return FromBytes({uiBytes, 16}, uiScopeId, out_address);
  }
  return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
}

xiiNetworkError xiiNetworkResolver::Resolve(xiiStringView sHost, xiiStringView sService,
                                            xiiDynamicArray<xiiNetworkEndpoint>& out_endpoints, xiiNetworkAddressFamily family, xiiNetworkTransport transport)
{
  using namespace xiiNetworkInternal;
  out_endpoints.Clear();
  const int iFamily = GetNativeFamily(family);
  if (iFamily < 0 || (transport != xiiNetworkTransport::Tcp && transport != xiiNetworkTransport::Udp) || ContainsNull(sHost) || ContainsNull(sService))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  RuntimeLease runtime;
  if (runtime.GetError().Failed())
  {
    return runtime.GetError();
  }
  const xiiStringBuilder sHostStorage(sHost);
  const xiiStringBuilder sServiceStorage(sService);
  addrinfo               hints = {};
  hints.ai_family              = iFamily;
  hints.ai_socktype            = transport == xiiNetworkTransport::Tcp ? SOCK_STREAM : SOCK_DGRAM;
  hints.ai_protocol            = transport == xiiNetworkTransport::Tcp ? IPPROTO_TCP : IPPROTO_UDP;
  hints.ai_flags               = sHost.IsEmpty() ? AI_PASSIVE : 0;
  addrinfo* pResults           = nullptr;
  const int iResult            = ::getaddrinfo(sHost.IsEmpty() ? nullptr : sHostStorage.GetData(), sService.IsEmpty() ? "0" : sServiceStorage.GetData(), &hints, &pResults);
  if (iResult != 0)
  {
    return TranslateResolverError(iResult, GetLastError());
  }
  // Keep OS allocations scoped even when entries are filtered or future validation adds early returns.
  struct ResolverOwner
  {
    addrinfo* m_pResults;
    ~ResolverOwner() { ::freeaddrinfo(m_pResults); }
  } owner{pResults};
  for (const addrinfo* pEntry = pResults; pEntry != nullptr; pEntry = pEntry->ai_next)
  {
    if (pEntry->ai_addr == nullptr || (pEntry->ai_family != AF_INET && pEntry->ai_family != AF_INET6))
    {
      continue;
    }
    xiiNetworkEndpoint    endpoint;
    const xiiNetworkError error = FromNative(pEntry->ai_addr, static_cast<NativeLength>(pEntry->ai_addrlen), endpoint);
    if (error.Succeeded() && !out_endpoints.Contains(endpoint))
    {
      out_endpoints.PushBack(endpoint);
    }
  }
  return out_endpoints.IsEmpty() ? xiiNetworkError(xiiNetworkErrorCode::NameNotFound) : xiiNetworkError();
}

xiiNetworkError xiiNetworkResolver::ReverseLookup(const xiiNetworkEndpoint& endpoint, xiiStringBuilder& out_sHostname)
{
  using namespace xiiNetworkInternal;
  NativeAddress         native;
  const xiiNetworkError error = ToNative(endpoint, native);
  if (error.Failed())
  {
    return error;
  }
  RuntimeLease runtime;
  if (runtime.GetError().Failed())
  {
    return runtime.GetError();
  }
  char      szHostname[NI_MAXHOST];
  const int iResult = ::getnameinfo(native.Get(), native.m_uiLength, szHostname, sizeof(szHostname), nullptr, 0, NI_NAMEREQD);
  if (iResult != 0)
  {
    return TranslateResolverError(iResult, GetLastError());
  }
  out_sHostname = szHostname;
  return {};
}

xiiNetworkSocket::~xiiNetworkSocket()
{
  Close().IgnoreResult();
}

xiiNetworkSocket::xiiNetworkSocket(xiiNetworkSocket&& other) noexcept
{
  *this = std::move(other);
}

xiiNetworkSocket& xiiNetworkSocket::operator=(xiiNetworkSocket&& other) noexcept
{
  if (this != &other)
  {
    // Swap ownership first so a close error cannot lose the destination's old resource.
    std::swap(m_uiNativeHandle, other.m_uiNativeHandle);
    std::swap(m_Family, other.m_Family);
    std::swap(m_bNonBlocking, other.m_bNonBlocking);
    std::swap(m_bDatagram, other.m_bDatagram);
    std::swap(m_State, other.m_State);
    std::swap(m_ConnectError, other.m_ConnectError);
    other.Close().IgnoreResult();
  }
  return *this;
}

xiiNetworkError xiiNetworkSocket::Close()
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return {};
  }
  const NativeSocket hSocket      = Borrow(*this);
  const int          iResult      = CloseNative(hSocket);
  const int          iNativeError = iResult == 0 ? 0 : GetLastError();
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (iResult != 0)
  {
    return TranslateSocketError(iNativeError);
  }
#endif
  m_uiNativeHandle = InvalidHandle;
  m_Family         = xiiNetworkAddressFamily::Unspecified;
  m_bNonBlocking   = false;
  m_bDatagram      = false;
  m_State          = xiiNetworkSocketState::Closed;
  m_ConnectError   = {};
  ReleaseRuntime();
  // POSIX close is deliberately never retried: Linux releases the descriptor even on EINTR.
  return TranslateSocketError(iNativeError);
}

xiiNetworkError xiiNetworkSocket::OpenSocket(xiiNetworkAddressFamily family, bool bDatagram, bool bNonBlocking)
{
  using namespace xiiNetworkInternal;
  if (IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (family != xiiNetworkAddressFamily::Ipv4 && family != xiiNetworkAddressFamily::Ipv6)
  {
    return xiiNetworkError(xiiNetworkErrorCode::UnsupportedAddressFamily);
  }
  RuntimeLease runtime;
  if (runtime.GetError().Failed())
  {
    return runtime.GetError();
  }
  xiiNetworkError    error;
  const NativeSocket hSocket = CreateNative(GetNativeFamily(family), bDatagram ? SOCK_DGRAM : SOCK_STREAM, bNonBlocking, error);
  if (hSocket == InvalidNativeSocket)
  {
    return error;
  }
  if (family == xiiNetworkAddressFamily::Ipv6)
  {
    const int iEnabled = 1;
    if (SetOption(hSocket, IPPROTO_IPV6, IPV6_V6ONLY, &iEnabled, sizeof(iEnabled)) != 0)
    {
      error = TranslateSocketError(GetLastError());
      CloseNative(hSocket);
      return error;
    }
  }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (!bDatagram)
  {
    // Keep accepted and newly-created sockets on the same deterministic ordinary-close policy.
    const linger value = {};
    if (SetOption(hSocket, SOL_SOCKET, SO_LINGER, &value, sizeof(value)) != 0)
    {
      error = TranslateSocketError(GetLastError());
      CloseNative(hSocket);
      return error;
    }
  }
#endif
  m_uiNativeHandle = static_cast<xiiUInt64>(hSocket);
  m_Family         = family;
  m_bNonBlocking   = bNonBlocking;
  m_bDatagram      = bDatagram;
  m_State          = xiiNetworkSocketState::Open;
  m_ConnectError   = {};
  runtime.Detach();
  return {};
}

xiiNetworkError xiiNetworkSocket::BindSocket(const xiiNetworkEndpoint& endpoint)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (!endpoint.IsValid() || endpoint.GetAddress().GetFamily() != m_Family)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  NativeAddress         native;
  const xiiNetworkError error = ToNative(endpoint, native);
  if (error.Failed())
  {
    return error;
  }
  return ::bind(Borrow(*this), native.Get(), native.m_uiLength) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
}

xiiNetworkError xiiNetworkSocket::ConnectSocket(const xiiNetworkEndpoint& endpoint)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (!endpoint.IsValid() || endpoint.GetAddress().GetFamily() != m_Family)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
  }
  NativeAddress         native;
  const xiiNetworkError error = ToNative(endpoint, native);
  if (error.Failed())
  {
    return error;
  }
  if (::connect(Borrow(*this), native.Get(), native.m_uiLength) == 0)
  {
    m_State = xiiNetworkSocketState::Connected;
    return {};
  }
  xiiNetworkError connectError = TranslateSocketError(GetLastError());
  if (connectError.m_Code == xiiNetworkErrorCode::WouldBlock || connectError.m_Code == xiiNetworkErrorCode::InProgress || connectError.m_Code == xiiNetworkErrorCode::Interrupted)
  {
    connectError.m_Code = xiiNetworkErrorCode::InProgress;
    m_State             = xiiNetworkSocketState::Connecting;
  }
  else
  {
    m_State = xiiNetworkSocketState::Failed;
  }
  m_ConnectError = connectError;
  return connectError;
}

xiiNetworkError xiiNetworkSocket::GetLocalEndpoint(xiiNetworkEndpoint& out_endpoint) const
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  NativeAddress native;
  if (::getsockname(Borrow(*this), native.Get(), &native.m_uiLength) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  return FromNative(native.Get(), native.m_uiLength, out_endpoint);
}

xiiNetworkError xiiNetworkSocket::GetPeerEndpoint(xiiNetworkEndpoint& out_endpoint) const
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  NativeAddress native;
  if (::getpeername(Borrow(*this), native.Get(), &native.m_uiLength) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  return FromNative(native.Get(), native.m_uiLength, out_endpoint);
}

xiiNetworkError xiiNetworkSocket::SetNonBlocking(bool bNonBlocking)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  const xiiNetworkError error = SetNonBlockingNative(Borrow(*this), bNonBlocking);
  if (error.Succeeded())
  {
    m_bNonBlocking = bNonBlocking;
  }
  return error;
}

xiiNetworkError xiiNetworkSocket::SetAddressReuse(bool bEnable) { return xiiNetworkInternal::SetIntegerOption(*this, SOL_SOCKET, SO_REUSEADDR, bEnable ? 1 : 0); }
xiiNetworkError xiiNetworkSocket::SetIpv6Only(bool bEnable)
{
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (m_Family != xiiNetworkAddressFamily::Ipv6)
  {
    return xiiNetworkError(xiiNetworkErrorCode::UnsupportedAddressFamily);
  }
  return xiiNetworkInternal::SetIntegerOption(*this, IPPROTO_IPV6, IPV6_V6ONLY, bEnable ? 1 : 0);
}

xiiNetworkError xiiNetworkSocket::SetSendBufferSize(xiiUInt32 uiBytes)
{
  if (uiBytes == 0 || uiBytes > INT_MAX)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  return xiiNetworkInternal::SetIntegerOption(*this, SOL_SOCKET, SO_SNDBUF, static_cast<int>(uiBytes));
}
xiiNetworkError xiiNetworkSocket::SetReceiveBufferSize(xiiUInt32 uiBytes)
{
  if (uiBytes == 0 || uiBytes > INT_MAX)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  return xiiNetworkInternal::SetIntegerOption(*this, SOL_SOCKET, SO_RCVBUF, static_cast<int>(uiBytes));
}
xiiNetworkError xiiNetworkSocket::GetBufferSizes(xiiUInt32& out_uiSendBytes, xiiUInt32& out_uiReceiveBytes) const
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  int          iSendSize    = 0;
  int          iReceiveSize = 0;
  NativeLength uiLength     = sizeof(int);
  if (GetOption(Borrow(*this), SOL_SOCKET, SO_SNDBUF, &iSendSize, &uiLength) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  uiLength = sizeof(int);
  if (GetOption(Borrow(*this), SOL_SOCKET, SO_RCVBUF, &iReceiveSize, &uiLength) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  if (iSendSize < 0 || iReceiveSize < 0)
  {
    return xiiNetworkError(xiiNetworkErrorCode::Unknown);
  }
  out_uiSendBytes    = static_cast<xiiUInt32>(iSendSize);
  out_uiReceiveBytes = static_cast<xiiUInt32>(iReceiveSize);
  return {};
}
xiiNetworkError xiiNetworkSocket::SetSendTimeout(xiiTime timeout) { return xiiNetworkInternal::SetTimeout(*this, SO_SNDTIMEO, timeout); }
xiiNetworkError xiiNetworkSocket::SetReceiveTimeout(xiiTime timeout) { return xiiNetworkInternal::SetTimeout(*this, SO_RCVTIMEO, timeout); }

xiiNetworkError xiiNetworkSocket::Wait(xiiNetworkSocketEvent event, xiiTime timeout) const
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (event != xiiNetworkSocketEvent::Readable && event != xiiNetworkSocketEvent::Writable)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiUInt32             uiMilliseconds;
  const xiiNetworkError error = Milliseconds(timeout, uiMilliseconds);
  if (error.Failed())
  {
    return error;
  }
  return WaitNative(Borrow(*this), event == xiiNetworkSocketEvent::Readable, static_cast<int>(uiMilliseconds));
}

xiiNetworkError xiiTcpSocket::Connect(const xiiNetworkEndpoint& endpoint)
{
  if (m_State == xiiNetworkSocketState::Connecting)
  {
    return m_ConnectError;
  }
  if (m_State == xiiNetworkSocketState::Failed)
  {
    return m_ConnectError;
  }
  if (m_State == xiiNetworkSocketState::Connected)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyConnected);
  }
  return ConnectSocket(endpoint);
}

xiiNetworkError xiiNetworkSocket::FinishConnection()
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (m_State == xiiNetworkSocketState::Connected)
  {
    return {};
  }
  if (m_State == xiiNetworkSocketState::Failed)
  {
    return m_ConnectError;
  }
  if (m_State != xiiNetworkSocketState::Connecting)
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotConnected);
  }
  const xiiNetworkError ready = Wait(xiiNetworkSocketEvent::Writable, xiiTime::MakeZero());
  if (ready.m_Code == xiiNetworkErrorCode::TimedOut)
  {
    return m_ConnectError;
  }
  if (ready.Failed())
  {
    return ready;
  }
  int          iPendingError = 0;
  NativeLength uiLength      = sizeof(iPendingError);
  if (GetOption(Borrow(*this), SOL_SOCKET, SO_ERROR, &iPendingError, &uiLength) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  m_ConnectError = TranslateSocketError(iPendingError);
  m_State        = m_ConnectError.Succeeded() ? xiiNetworkSocketState::Connected : xiiNetworkSocketState::Failed;
  return m_ConnectError;
}

xiiNetworkError xiiTcpSocket::Shutdown(xiiNetworkShutdown direction)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  int iDirection;
  switch (direction)
  {
    case xiiNetworkShutdown::Receive:
      iDirection = 0;
      break;
    case xiiNetworkShutdown::Send:
      iDirection = 1;
      break;
    case xiiNetworkShutdown::Both:
      iDirection = 2;
      break;
    default:
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  return ::shutdown(Borrow(*this), iDirection) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
}
xiiNetworkError xiiTcpSocket::SetNoDelay(bool bEnable) { return xiiNetworkInternal::SetIntegerOption(*this, IPPROTO_TCP, TCP_NODELAY, bEnable ? 1 : 0); }
xiiNetworkError xiiTcpSocket::SetKeepAlive(bool bEnable) { return xiiNetworkInternal::SetIntegerOption(*this, SOL_SOCKET, SO_KEEPALIVE, bEnable ? 1 : 0); }
xiiNetworkError xiiTcpSocket::SetLinger(bool bDiscardPendingData)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  linger value  = {};
  value.l_onoff = bDiscardPendingData ? 1 : 0;
  return SetOption(Borrow(*this), SOL_SOCKET, SO_LINGER, &value, sizeof(value)) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
}

xiiNetworkError xiiTcpSocket::SetKeepAliveTiming(xiiTime idleTime, xiiTime interval, xiiUInt32 uiProbeCount)
{
  const double fIdle     = idleTime.GetSeconds();
  const double fInterval = interval.GetSeconds();
  if (!std::isfinite(fIdle) || !std::isfinite(fInterval) || fIdle < 1 || fInterval < 1 ||
      fIdle > INT_MAX || fInterval > INT_MAX || std::floor(fIdle) != fIdle || std::floor(fInterval) != fInterval || uiProbeCount == 0 || uiProbeCount > INT_MAX)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
#if defined(TCP_KEEPCNT) && defined(TCP_KEEPINTVL) && (defined(TCP_KEEPIDLE) || defined(TCP_KEEPALIVE))
#  if defined(TCP_KEEPIDLE)
  constexpr int IdleOption = TCP_KEEPIDLE;
#  else
  constexpr int IdleOption = TCP_KEEPALIVE;
#  endif
  xiiNetworkError error = xiiNetworkInternal::SetIntegerOption(*this, IPPROTO_TCP, IdleOption, static_cast<int>(fIdle));
  if (error.Failed())
  {
    return error;
  }
  error = xiiNetworkInternal::SetIntegerOption(*this, IPPROTO_TCP, TCP_KEEPINTVL, static_cast<int>(fInterval));
  if (error.Failed())
  {
    return error;
  }
  return xiiNetworkInternal::SetIntegerOption(*this, IPPROTO_TCP, TCP_KEEPCNT, static_cast<int>(uiProbeCount));
#else
  return xiiNetworkError(xiiNetworkErrorCode::UnsupportedOperation);
#endif
}

xiiNetworkError xiiTcpListener::Listen(xiiUInt32 uiBacklog)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (uiBacklog == 0 || uiBacklog > INT_MAX)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (::listen(Borrow(*this), static_cast<int>(uiBacklog)) != 0)
  {
    return TranslateSocketError(GetLastError());
  }
  m_State = xiiNetworkSocketState::Listening;
  return {};
}

xiiNetworkError xiiTcpListener::Accept(xiiTcpSocket& out_socket, xiiNetworkEndpoint* out_pPeer, bool bNonBlocking)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (out_socket.IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (m_State != xiiNetworkSocketState::Listening)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  RuntimeLease runtime;
  if (runtime.GetError().Failed())
  {
    return runtime.GetError();
  }
  NativeAddress      native;
  xiiNetworkError    error;
  const NativeSocket hSocket = AcceptNative(Borrow(*this), native.Get(), &native.m_uiLength, bNonBlocking, error);
  if (hSocket == InvalidNativeSocket)
  {
    return error;
  }
  xiiNetworkEndpoint peer;
  error = FromNative(native.Get(), native.m_uiLength, peer);
  if (error.Failed())
  {
    CloseNative(hSocket);
    return error;
  }
  out_socket.m_uiNativeHandle = static_cast<xiiUInt64>(hSocket);
  out_socket.m_Family         = GetFamily();
  out_socket.m_bNonBlocking   = bNonBlocking;
  out_socket.m_bDatagram      = false;
  out_socket.m_State          = xiiNetworkSocketState::Connected;
  out_socket.m_ConnectError   = {};
  runtime.Detach();
  if (out_pPeer != nullptr)
  {
    *out_pPeer = peer;
  }
  return {};
}

xiiNetworkOperationResult xiiNetworkSocket::SendBuffers(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers, const xiiNetworkEndpoint* pDestination)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return {xiiNetworkError(xiiNetworkErrorCode::NotOpen), 0};
  }
  if (!m_bDatagram && m_State != xiiNetworkSocketState::Connected)
  {
    return {xiiNetworkError(m_State == xiiNetworkSocketState::Connecting ? xiiNetworkErrorCode::InProgress : xiiNetworkErrorCode::NotConnected), 0};
  }
  if (pDestination == nullptr && m_bDatagram && m_State != xiiNetworkSocketState::Connected)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::NotConnected), 0};
  }
  xiiUInt32       uiTotal;
  xiiNetworkError error = ValidateBuffers(buffers, m_bDatagram, m_Family, uiTotal);
  if (error.Failed())
  {
    return {error, 0};
  }
  if (!m_bDatagram && uiTotal == 0)
  {
    return {};
  }
  NativeAddress destination;
  if (pDestination != nullptr)
  {
    if (pDestination->GetAddress().GetFamily() != m_Family)
    {
      return {xiiNetworkError(xiiNetworkErrorCode::UnsupportedAddressFamily), 0};
    }
    error = ToNative(*pDestination, destination);
    if (error.Failed())
    {
      return {error, 0};
    }
  }
  xiiUInt32 uiTransferred = 0;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  WSABUF nativeBuffers[64] = {};
  DWORD  uiCount           = 0;
  char   cEmpty            = 0;
  for (const auto& bytes : buffers)
  {
    if (!bytes.IsEmpty())
    {
      nativeBuffers[uiCount].buf = reinterpret_cast<char*>(const_cast<xiiUInt8*>(bytes.GetPtr()));
      nativeBuffers[uiCount].len = bytes.GetCount();
      ++uiCount;
    }
  }
  if (uiCount == 0)
  {
    nativeBuffers[0].buf = &cEmpty;
    uiCount              = 1;
  }
  DWORD     uiSent  = 0;
  const int iResult = pDestination != nullptr ?
    ::WSASendTo(Borrow(*this), nativeBuffers, uiCount, &uiSent, 0, destination.Get(), destination.m_uiLength, nullptr, nullptr) :
    ::WSASend(Borrow(*this), nativeBuffers, uiCount, &uiSent, 0, nullptr, nullptr);
  if (iResult != 0)
  {
    return {TranslateSocketError(GetLastError()), 0};
  }
  uiTransferred = uiSent;
#else
  iovec    nativeBuffers[64] = {};
  size_t   uiCount           = 0;
  xiiUInt8 uiEmpty           = 0;
  for (const auto& bytes : buffers)
  {
    if (!bytes.IsEmpty())
    {
      nativeBuffers[uiCount].iov_base = const_cast<xiiUInt8*>(bytes.GetPtr());
      nativeBuffers[uiCount].iov_len  = bytes.GetCount();
      ++uiCount;
    }
  }
  if (uiCount == 0)
  {
    nativeBuffers[0].iov_base = &uiEmpty;
    uiCount                   = 1;
  }
  msghdr message     = {};
  message.msg_iov    = nativeBuffers;
  message.msg_iovlen = static_cast<decltype(message.msg_iovlen)>(uiCount);
  if (pDestination != nullptr)
  {
    message.msg_name    = destination.Get();
    message.msg_namelen = destination.m_uiLength;
  }
#  if XII_ENABLED(XII_PLATFORM_LINUX)
  constexpr int SendFlags = MSG_NOSIGNAL;
#  else
  constexpr int SendFlags = 0; // SO_NOSIGPIPE was established on creation/accept on macOS.
#  endif
  const ssize_t iResult = ::sendmsg(Borrow(*this), &message, SendFlags);
  if (iResult < 0)
  {
    return {TranslateSocketError(GetLastError()), 0};
  }
  uiTransferred = static_cast<xiiUInt32>(iResult);
#endif
  if (m_bDatagram && uiTransferred != uiTotal)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::Unknown), uiTransferred};
  }
  return {{}, uiTransferred};
}

xiiNetworkOperationResult xiiNetworkSocket::ReceiveBuffers(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> buffers, xiiNetworkEndpoint* out_pSender)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return {xiiNetworkError(xiiNetworkErrorCode::NotOpen), 0};
  }
  if (!m_bDatagram && m_State != xiiNetworkSocketState::Connected)
  {
    return {xiiNetworkError(m_State == xiiNetworkSocketState::Connecting ? xiiNetworkErrorCode::InProgress : xiiNetworkErrorCode::NotConnected), 0};
  }
  xiiUInt32       uiTotal;
  xiiNetworkError error = ValidateBuffers(buffers, false, m_Family, uiTotal);
  if (error.Failed())
  {
    return {error, 0};
  }
  if (!m_bDatagram && uiTotal == 0)
  {
    return {};
  }
  NativeAddress sender;
  xiiUInt32     uiTransferred;
  bool          bTruncated      = false;
  xiiInt32      iTruncationCode = 0;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  WSABUF nativeBuffers[64] = {};
  DWORD  uiCount           = 0;
  char   cEmpty            = 0;
  for (const auto& bytes : buffers)
  {
    if (!bytes.IsEmpty())
    {
      nativeBuffers[uiCount].buf = reinterpret_cast<char*>(bytes.GetPtr());
      nativeBuffers[uiCount].len = bytes.GetCount();
      ++uiCount;
    }
  }
  if (uiCount == 0)
  {
    nativeBuffers[0].buf = &cEmpty;
    uiCount              = 1;
  }
  DWORD     uiReceived = 0;
  DWORD     uiFlags    = 0;
  const int iResult    = m_bDatagram ?
    ::WSARecvFrom(Borrow(*this), nativeBuffers, uiCount, &uiReceived, &uiFlags, sender.Get(), &sender.m_uiLength, nullptr, nullptr) :
    ::WSARecv(Borrow(*this), nativeBuffers, uiCount, &uiReceived, &uiFlags, nullptr, nullptr);
  if (iResult != 0)
  {
    const int iError = GetLastError();
    if (!m_bDatagram || iError != WSAEMSGSIZE)
    {
      return {TranslateSocketError(iError), 0};
    }
    // Winsock fills the supplied buffers and discards the tail of an oversized UDP datagram.
    bTruncated      = true;
    iTruncationCode = iError;
    uiReceived      = uiTotal;
  }
  bTruncated    = bTruncated || (uiFlags & MSG_PARTIAL) != 0;
  uiTransferred = uiReceived;
#else
  iovec    nativeBuffers[64] = {};
  size_t   uiCount           = 0;
  xiiUInt8 uiEmpty           = 0;
  for (const auto& bytes : buffers)
  {
    if (!bytes.IsEmpty())
    {
      nativeBuffers[uiCount].iov_base = bytes.GetPtr();
      nativeBuffers[uiCount].iov_len  = bytes.GetCount();
      ++uiCount;
    }
  }
  if (uiCount == 0)
  {
    nativeBuffers[0].iov_base = &uiEmpty;
    uiCount                   = 1;
  }
  msghdr message     = {};
  message.msg_iov    = nativeBuffers;
  message.msg_iovlen = static_cast<decltype(message.msg_iovlen)>(uiCount);
  if (m_bDatagram)
  {
    message.msg_name    = sender.Get();
    message.msg_namelen = sender.m_uiLength;
  }
  // No input MSG_TRUNC: return the bytes actually copied on Linux and BSD alike.
  const ssize_t iResult = ::recvmsg(Borrow(*this), &message, 0);
  if (iResult < 0)
  {
    return {TranslateSocketError(GetLastError()), 0};
  }
  uiTransferred     = static_cast<xiiUInt32>(iResult);
  bTruncated        = (message.msg_flags & MSG_TRUNC) != 0;
  sender.m_uiLength = message.msg_namelen;
#endif
  if (m_bDatagram && out_pSender != nullptr)
  {
    error = FromNative(sender.Get(), sender.m_uiLength, *out_pSender);
    if (error.Failed())
    {
      return {error, uiTransferred};
    }
  }
  if (bTruncated)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::Truncated, iTruncationCode,
                            iTruncationCode != 0 ? xiiNetworkErrorDomain::Socket : xiiNetworkErrorDomain::None),
            uiTransferred};
  }
  if (!m_bDatagram && uiTransferred == 0)
  {
    return {xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed), 0};
  }
  return {{}, uiTransferred};
}

namespace xiiNetworkInternal
{
  xiiNetworkError MulticastMembership(const xiiNetworkSocket& socket, const xiiNetworkAddress& group, const xiiNetworkAddress& ipv4Interface, xiiUInt32 uiIpv6InterfaceIndex, bool bJoin)
  {
    if (!socket.IsOpen())
    {
      return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
    }
    if (!group.IsMulticast() || group.GetFamily() != socket.GetFamily())
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
    }
    if (socket.GetFamily() == xiiNetworkAddressFamily::Ipv4)
    {
      if (ipv4Interface.GetFamily() != xiiNetworkAddressFamily::Ipv4 || uiIpv6InterfaceIndex != 0)
      {
        return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
      }
      ip_mreq membership = {};
      std::memcpy(&membership.imr_multiaddr, group.GetBytes().GetPtr(), 4);
      std::memcpy(&membership.imr_interface, ipv4Interface.GetBytes().GetPtr(), 4);
      return SetOption(Borrow(socket), IPPROTO_IP, bJoin ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &membership, sizeof(membership)) == 0 ?
        xiiNetworkError() :
        TranslateSocketError(GetLastError());
    }
    if (group.GetScopeId() != 0 && uiIpv6InterfaceIndex != 0 && group.GetScopeId() != uiIpv6InterfaceIndex)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    ipv6_mreq membership = {};
    std::memcpy(&membership.ipv6mr_multiaddr, group.GetBytes().GetPtr(), 16);
    membership.ipv6mr_interface = uiIpv6InterfaceIndex != 0 ? uiIpv6InterfaceIndex : group.GetScopeId();
    return SetOption(Borrow(socket), IPPROTO_IPV6, bJoin ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, &membership, sizeof(membership)) == 0 ?
      xiiNetworkError() :
      TranslateSocketError(GetLastError());
  }
} // namespace xiiNetworkInternal

xiiNetworkError xiiUdpSocket::SetBroadcast(bool bEnable) { return xiiNetworkInternal::SetIntegerOption(*this, SOL_SOCKET, SO_BROADCAST, bEnable ? 1 : 0); }
xiiNetworkError xiiUdpSocket::JoinMulticast(const xiiNetworkAddress& group, const xiiNetworkAddress& ipv4Interface, xiiUInt32 uiIpv6InterfaceIndex)
{
  return xiiNetworkInternal::MulticastMembership(*this, group, ipv4Interface, uiIpv6InterfaceIndex, true);
}
xiiNetworkError xiiUdpSocket::LeaveMulticast(const xiiNetworkAddress& group, const xiiNetworkAddress& ipv4Interface, xiiUInt32 uiIpv6InterfaceIndex)
{
  return xiiNetworkInternal::MulticastMembership(*this, group, ipv4Interface, uiIpv6InterfaceIndex, false);
}
xiiNetworkError xiiUdpSocket::SetMulticastInterface(const xiiNetworkAddress& ipv4Interface, xiiUInt32 uiIpv6InterfaceIndex)
{
  using namespace xiiNetworkInternal;
  if (!IsOpen())
  {
    return xiiNetworkError(xiiNetworkErrorCode::NotOpen);
  }
  if (GetFamily() == xiiNetworkAddressFamily::Ipv4)
  {
    if (ipv4Interface.GetFamily() != xiiNetworkAddressFamily::Ipv4 || uiIpv6InterfaceIndex != 0)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
    in_addr native = {};
    std::memcpy(&native, ipv4Interface.GetBytes().GetPtr(), 4);
    return SetOption(Borrow(*this), IPPROTO_IP, IP_MULTICAST_IF, &native, sizeof(native)) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
  }
  const xiiUInt32 uiIndex = uiIpv6InterfaceIndex;
  return SetOption(Borrow(*this), IPPROTO_IPV6, IPV6_MULTICAST_IF, &uiIndex, sizeof(uiIndex)) == 0 ? xiiNetworkError() : TranslateSocketError(GetLastError());
}
xiiNetworkError xiiUdpSocket::SetMulticastHopLimit(xiiUInt8 uiHops)
{
#if XII_ENABLED(XII_PLATFORM_OSX)
  if (GetFamily() == xiiNetworkAddressFamily::Ipv4 && IsOpen())
  {
    return xiiNetworkInternal::SetOption(xiiNetworkInternal::Borrow(*this), IPPROTO_IP, IP_MULTICAST_TTL, &uiHops, sizeof(uiHops)) == 0 ?
      xiiNetworkError() :
      xiiNetworkInternal::TranslateSocketError(xiiNetworkInternal::GetLastError());
  }
#endif
  return xiiNetworkInternal::SetIntegerOption(*this, GetFamily() == xiiNetworkAddressFamily::Ipv4 ? IPPROTO_IP : IPPROTO_IPV6,
                                              GetFamily() == xiiNetworkAddressFamily::Ipv4 ? IP_MULTICAST_TTL : IPV6_MULTICAST_HOPS, uiHops);
}
xiiNetworkError xiiUdpSocket::SetMulticastLoopback(bool bEnable)
{
#if XII_ENABLED(XII_PLATFORM_OSX)
  // BSD requires a byte for IPv4 multicast loopback; IPv6 uses an integer.
  if (GetFamily() == xiiNetworkAddressFamily::Ipv4 && IsOpen())
  {
    const xiiUInt8 uiValue = bEnable ? 1 : 0;
    return xiiNetworkInternal::SetOption(xiiNetworkInternal::Borrow(*this), IPPROTO_IP, IP_MULTICAST_LOOP, &uiValue, sizeof(uiValue)) == 0 ?
      xiiNetworkError() :
      xiiNetworkInternal::TranslateSocketError(xiiNetworkInternal::GetLastError());
  }
#endif
  return xiiNetworkInternal::SetIntegerOption(*this, GetFamily() == xiiNetworkAddressFamily::Ipv4 ? IPPROTO_IP : IPPROTO_IPV6,
                                              GetFamily() == xiiNetworkAddressFamily::Ipv4 ? IP_MULTICAST_LOOP : IPV6_MULTICAST_LOOP, bEnable ? 1 : 0);
}
