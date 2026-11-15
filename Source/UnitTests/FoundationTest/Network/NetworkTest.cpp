/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Network/NetworkResolver.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Network/UdpSocket.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

#include <FoundationTest/Network/Implementation/NetworkTestUtilities.h>

#include <cstring>
#include <source_location>
#include <type_traits>
#include <utility>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
#  include <dirent.h>
#endif

XII_CREATE_SIMPLE_TEST_GROUP(Network);

namespace xiiNetworkTestUtilities
{
  xiiUInt32 CountOpenResources()
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    DWORD uiCount = 0;
    return ::GetProcessHandleCount(::GetCurrentProcess(), &uiCount) ? uiCount : 0;
#else
#  if XII_ENABLED(XII_PLATFORM_LINUX)
    DIR* pDirectory = ::opendir("/proc/self/fd");
#  else
    DIR* pDirectory = ::opendir("/dev/fd");
#  endif
    if (pDirectory == nullptr)
    {
      return 0;
    }
    xiiUInt32 uiCount = 0;
    while (const dirent* pEntry = ::readdir(pDirectory))
    {
      if (pEntry->d_name[0] != '.')
      {
        ++uiCount;
      }
    }
    ::closedir(pDirectory);
    return uiCount;
#endif
  }

} // namespace xiiNetworkTestUtilities

namespace
{
  const xiiTime TestTimeout = xiiTime::MakeFromSeconds(3);

  bool Check(xiiNetworkError error, const std::source_location& location = std::source_location::current())
  {
    return XII_TEST_BOOL_MSG(error.Succeeded(), "%s (native %d, domain %u) at %s:%u", error.GetCodeName(), error.m_iNativeCode, static_cast<xiiUInt32>(error.m_Domain), location.file_name(), location.line());
  }

  xiiNetworkEndpoint Loopback(xiiNetworkAddressFamily family)
  {
    return xiiNetworkEndpoint(xiiNetworkAddress::MakeLoopback(family), 0);
  }

  xiiNetworkError OpenListener(xiiTcpListener& out_listener, xiiNetworkEndpoint& out_endpoint, xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Ipv4)
  {
    xiiNetworkError error = out_listener.Open(family);
    if (error.Failed())
    {
      return error;
    }
    error = out_listener.Bind(Loopback(family));
    if (error.Failed())
    {
      return error;
    }
    error = out_listener.Listen();
    if (error.Failed())
    {
      return error;
    }
    return out_listener.GetLocalEndpoint(out_endpoint);
  }

  xiiNetworkError ConnectClient(xiiTcpSocket& out_client, const xiiNetworkEndpoint& endpoint)
  {
    xiiNetworkError error = out_client.Open(endpoint.GetAddress().GetFamily());
    if (error.Failed())
    {
      return error;
    }
    error = out_client.Connect(endpoint);
    if (error.m_Code == xiiNetworkErrorCode::InProgress)
    {
      error = out_client.Wait(xiiNetworkSocketEvent::Writable, TestTimeout);
      if (error.Failed())
      {
        return error;
      }
      return out_client.FinishConnect();
    }
    return error;
  }

  xiiNetworkError MakePair(xiiTcpSocket& out_client, xiiTcpSocket& out_server, xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Ipv4)
  {
    xiiTcpListener     listener;
    xiiNetworkEndpoint endpoint;
    xiiNetworkError    error = OpenListener(listener, endpoint, family);
    if (error.Failed())
    {
      return error;
    }
    error = ConnectClient(out_client, endpoint);
    if (error.Failed())
    {
      return error;
    }
    error = listener.Wait(xiiNetworkSocketEvent::Readable, TestTimeout);
    if (error.Failed())
    {
      return error;
    }
    return listener.Accept(out_server);
  }

  bool CanUseIpv6()
  {
    xiiUdpSocket    probe;
    xiiNetworkError error = probe.Open(xiiNetworkAddressFamily::Ipv6);
    if (error.Succeeded())
    {
      error = probe.Bind(Loopback(xiiNetworkAddressFamily::Ipv6));
    }
    if (error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily || error.m_Code == xiiNetworkErrorCode::AddressNotAvailable || error.m_Code == xiiNetworkErrorCode::UnsupportedOperation)
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped IPv6 loopback: %s (native %d)", error.GetCodeName(), error.m_iNativeCode);
      return false;
    }
    return Check(error);
  }

  bool TransferAll(xiiTcpSocket& sender, xiiTcpSocket& receiver, xiiArrayPtr<const xiiUInt8> input, xiiArrayPtr<xiiUInt8> output)
  {
    xiiUInt32     uiSent     = 0;
    xiiUInt32     uiReceived = 0;
    const xiiTime deadline   = xiiTime::Now() + TestTimeout;
    while (uiReceived < input.GetCount() && xiiTime::Now() < deadline)
    {
      if (uiSent < input.GetCount())
      {
        const auto result = sender.Send(input.GetSubArray(uiSent));
        if (result.Succeeded())
        {
          uiSent += result.m_uiBytesTransferred;
        }
        else if (result.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock && result.m_Error.m_Code != xiiNetworkErrorCode::Interrupted)
        {
          return Check(result.m_Error);
        }
      }
      const auto result = receiver.Receive(output.GetSubArray(uiReceived));
      if (result.Succeeded())
      {
        uiReceived += result.m_uiBytesTransferred;
      }
      else if (result.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock && result.m_Error.m_Code != xiiNetworkErrorCode::Interrupted)
      {
        return Check(result.m_Error);
      }
    }
    return XII_TEST_INT(uiSent, input.GetCount()) && XII_TEST_INT(uiReceived, input.GetCount()) && XII_TEST_BOOL(input == xiiArrayPtr<const xiiUInt8>(output));
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Network, Addresses)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IPv4 and IPv6 parsing, canonical formatting and binary round trips")
  {
    struct AddressCase
    {
      const char* m_szInput;
      const char* m_szCanonical;
    };
    const AddressCase cases[] = {
      {"127.0.0.1", "127.0.0.1"},
      {"0.0.0.0", "0.0.0.0"},
      {"255.255.255.255", "255.255.255.255"},
      {"::", "::"},
      {"::1", "::1"},
      {"2001:0DB8:0:0:1:0:0:1", "2001:db8::1:0:0:1"},
      {"1:0:0:2:0:0:3:4", "1::2:0:0:3:4"},
      {"1:2:3:4:5:6:0:0", "1:2:3:4:5:6::"},
      {"::ffff:192.0.2.1", "::ffff:c000:201"},
      {"fe80::1%7", "fe80::1%7"},
      {"fe80::1%4294967295", "fe80::1%4294967295"}};
    for (const auto& testCase : cases)
    {
      xiiNetworkAddress address;
      if (!Check(xiiNetworkAddress::Parse(testCase.m_szInput, address)))
      {
        return;
      }
      xiiStringBuilder sText;
      address.ToString(sText);
      XII_TEST_STRING(sText, testCase.m_szCanonical);
      xiiNetworkAddress roundTrip;
      Check(xiiNetworkAddress::Parse(sText, roundTrip));
      XII_TEST_BOOL(roundTrip == address);
      Check(xiiNetworkAddress::FromBytes(address.GetBytes(), address.GetScopeId(), roundTrip));
      XII_TEST_BOOL(roundTrip == address);
      XII_TEST_INT(roundTrip.ComputeHash(), address.ComputeHash());
    }
    const char* szInvalid[] = {"", "localhost", "1.2.3", "256.0.0.1", "1.2.3.4:5", "[::1]", "1::2::3", ":::1", "::1%", "::1%4294967296", "::1%1%2", "127.0.0.1%1", " ::1", "::1 "};
    for (const char* szInput : szInvalid)
    {
      xiiNetworkAddress address = xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4);
      XII_TEST_BOOL(xiiNetworkAddress::Parse(szInput, address).Failed());
      XII_TEST_BOOL(address == xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4));
    }
#if XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
    xiiNetworkAddress scoped;
#  if XII_ENABLED(XII_PLATFORM_LINUX)
    Check(xiiNetworkAddress::Parse("fe80::1%lo", scoped));
#  else
    Check(xiiNetworkAddress::Parse("fe80::1%lo0", scoped));
#  endif
    XII_TEST_BOOL(scoped.GetScopeId() != 0);
    xiiStringBuilder sScoped;
    scoped.ToString(sScoped);
    xiiNetworkAddress scopedRoundTrip;
    Check(xiiNetworkAddress::Parse(sScoped, scopedRoundTrip));
    XII_TEST_BOOL(scopedRoundTrip == scoped);
#endif
    const char        szEmbeddedNull[] = {'1', '.', '2', '.', '3', '.', '4', '\0', '5'};
    xiiNetworkAddress address;
    XII_TEST_BOOL(xiiNetworkAddress::Parse(xiiStringView(szEmbeddedNull, 9), address).Failed());
    XII_TEST_BOOL(!address.IsValid());
    XII_TEST_BOOL(!address.IsWildcard());
    XII_TEST_BOOL(xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv4).IsWildcard());
    XII_TEST_BOOL(xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv6).IsWildcard());
    XII_TEST_BOOL(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4).IsLoopback());
    XII_TEST_BOOL(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv6).IsLoopback());
    Check(xiiNetworkAddress::Parse("239.1.2.3", address));
    XII_TEST_BOOL(address.IsMulticast());
    Check(xiiNetworkAddress::Parse("ff02::1%7", address));
    XII_TEST_BOOL(address.IsMulticast());
    xiiNetworkAddress other;
    Check(xiiNetworkAddress::Parse("ff02::1%8", other));
    XII_TEST_BOOL(address != other);
    xiiHashTable<xiiNetworkAddress, xiiUInt32> table;
    table.Insert(address, 7);
    table.Insert(other, 8);
    XII_TEST_INT(table.GetCount(), 2);
    XII_TEST_INT(table[address], 7);
    XII_TEST_INT(table[other], 8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Endpoint syntax and port boundaries")
  {
    const char* szEndpoints[] = {"127.0.0.1:0", "127.0.0.1:65535", "[::1]:8080", "[2001:db8::1]:443", "[fe80::1%7]:9"};
    for (const char* szInput : szEndpoints)
    {
      xiiNetworkEndpoint endpoint;
      Check(xiiNetworkEndpoint::Parse(szInput, endpoint));
      xiiStringBuilder sText;
      endpoint.ToString(sText);
      XII_TEST_STRING(sText, szInput);
      xiiNetworkEndpoint roundTrip;
      Check(xiiNetworkEndpoint::Parse(sText, roundTrip));
      XII_TEST_BOOL(roundTrip == endpoint);
      XII_TEST_INT(roundTrip.ComputeHash(), endpoint.ComputeHash());
    }
    const char* szInvalid[] = {"127.0.0.1", "::1:80", "[::1]", "[::1]:", "[::1]:65536", "127.0.0.1:-1", "127.0.0.1:65536", "127.0.0.1:999999999999999999999", "[127.0.0.1]:80", "127.0.0.1:1x"};
    for (const char* szInput : szInvalid)
    {
      xiiNetworkEndpoint endpoint;
      XII_TEST_BOOL(xiiNetworkEndpoint::Parse(szInput, endpoint).Failed());
      XII_TEST_BOOL(!endpoint.IsValid());
    }
    xiiHashTable<xiiNetworkEndpoint, xiiUInt32> table;
    const xiiNetworkEndpoint                    first(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 1);
    const xiiNetworkEndpoint                    second(first.GetAddress(), 2);
    table.Insert(first, 1);
    table.Insert(second, 2);
    XII_TEST_INT(table.GetCount(), 2);
  }
}

XII_CREATE_SIMPLE_TEST(Network, Resolver)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Local resolution, filtering, service and reverse lookup")
  {
    xiiDynamicArray<xiiNetworkEndpoint> endpoints;
    if (!Check(xiiNetworkResolver::Resolve("localhost", "80", endpoints)))
    {
      return;
    }
    XII_TEST_BOOL(!endpoints.IsEmpty());
    for (xiiUInt32 uiIndex = 0; uiIndex < endpoints.GetCount(); ++uiIndex)
    {
      XII_TEST_INT(endpoints[uiIndex].GetPort(), 80);
      XII_TEST_BOOL(endpoints[uiIndex].GetAddress().IsLoopback());
      for (xiiUInt32 uiOther = 0; uiOther < uiIndex; ++uiOther)
      {
        XII_TEST_BOOL(endpoints[uiIndex] != endpoints[uiOther]);
      }
    }
    Check(xiiNetworkResolver::Resolve("127.0.0.1", "http", endpoints, xiiNetworkAddressFamily::Ipv4));
    XII_TEST_INT(endpoints.GetCount(), 1);
    if (!endpoints.IsEmpty())
    {
      XII_TEST_INT(endpoints[0].GetPort(), 80);
    }
    Check(xiiNetworkResolver::Resolve("", "0", endpoints));
    XII_TEST_BOOL(!endpoints.IsEmpty());
    for (const auto& endpoint : endpoints)
    {
      XII_TEST_BOOL(endpoint.GetAddress().IsWildcard());
    }
    if (CanUseIpv6())
    {
      Check(xiiNetworkResolver::Resolve("::1", "53", endpoints, xiiNetworkAddressFamily::Ipv6, xiiNetworkTransport::Udp));
      XII_TEST_INT(endpoints.GetCount(), 1);
      if (!endpoints.IsEmpty())
      {
        XII_TEST_BOOL(endpoints[0].GetAddress().GetFamily() == xiiNetworkAddressFamily::Ipv6);
        XII_TEST_INT(endpoints[0].GetPort(), 53);
      }
    }
    xiiStringBuilder sHostname;
    const auto       reverse = xiiNetworkResolver::ReverseLookup(Loopback(xiiNetworkAddressFamily::Ipv4), sHostname);
    if (reverse.Succeeded())
    {
      XII_TEST_BOOL(!sHostname.IsEmpty());
    }
    else
    {
      XII_TEST_BOOL(reverse.m_Code == xiiNetworkErrorCode::NameNotFound);
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped reverse localhost name: host database has no reverse entry.");
    }
    // Malformed hostname input is rejected before invoking the OS resolver.
    const char szInvalidHost[] = {'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', '\0', 'x'};
    const auto invalidHost     = xiiNetworkResolver::Resolve(xiiStringView(szInvalidHost, 11), "80", endpoints);
    XII_TEST_BOOL(invalidHost.m_Code == xiiNetworkErrorCode::InvalidArgument);
    XII_TEST_BOOL(endpoints.IsEmpty());
    // A numeric host with an unknown local service exercises native resolver error translation
    // without issuing an invalid-host DNS query or depending on an Internet DNS server.
    const auto invalid = xiiNetworkResolver::Resolve("127.0.0.1", "xii-invalid-network-service", endpoints);
    XII_TEST_BOOL(invalid.Failed());
    XII_TEST_BOOL(invalid.m_Domain == xiiNetworkErrorDomain::Resolver);
    XII_TEST_BOOL(invalid.m_iNativeCode != 0);
    XII_TEST_BOOL(endpoints.IsEmpty());
    for (xiiUInt32 uiIndex = 0; uiIndex < 64; ++uiIndex)
    {
      Check(xiiNetworkResolver::Resolve("127.0.0.1", "0", endpoints));
    }
  }
}

XII_CREATE_SIMPLE_TEST(Network, Tcp)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nonblocking connect, multiple acceptance and endpoint conversion")
  {
    xiiTcpListener     listener;
    xiiNetworkEndpoint endpoint;
    if (!Check(OpenListener(listener, endpoint)))
    {
      return;
    }
    XII_TEST_BOOL(endpoint.GetPort() != 0);
    xiiTcpSocket unused;
    XII_TEST_BOOL(listener.Accept(unused).m_Code == xiiNetworkErrorCode::WouldBlock);
    XII_TEST_BOOL(!unused.IsOpen());
    xiiTcpSocket clients[4];
    xiiTcpSocket servers[4];
    for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
    {
      if (!Check(ConnectClient(clients[uiIndex], endpoint)) || !Check(listener.Wait(xiiNetworkSocketEvent::Readable, TestTimeout)))
      {
        return;
      }
      xiiNetworkEndpoint peer;
      if (!Check(listener.Accept(servers[uiIndex], &peer)))
      {
        return;
      }
      xiiNetworkEndpoint local;
      Check(clients[uiIndex].GetLocalEndpoint(local));
      XII_TEST_BOOL(peer == local);
      Check(clients[uiIndex].GetPeerEndpoint(peer));
      XII_TEST_BOOL(peer == endpoint);
      XII_TEST_BOOL(clients[uiIndex].GetState() == xiiNetworkSocketState::Connected);
      XII_TEST_BOOL(servers[uiIndex].IsNonBlocking());
      XII_TEST_BOOL(listener.Accept(servers[uiIndex]).m_Code == xiiNetworkErrorCode::AlreadyOpen);
    }
    Check(listener.Close());
    const xiiUInt8 uiMessage[] = {1, 2, 3, 4};
    xiiUInt8       uiReceived[4];
    for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
    {
      TransferAll(clients[uiIndex], servers[uiIndex], uiMessage, uiReceived);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Large payload, backpressure and partial transfers")
  {
    xiiTcpSocket client;
    xiiTcpSocket server;
    if (!Check(MakePair(client, server)))
    {
      return;
    }
    Check(client.SetSendBufferSize(4096));
    xiiDynamicArray<xiiUInt8> input;
    xiiDynamicArray<xiiUInt8> output;
    input.SetCountUninitialized(2 * 1024 * 1024);
    output.SetCountUninitialized(input.GetCount());
    for (xiiUInt32 uiIndex = 0; uiIndex < input.GetCount(); ++uiIndex)
    {
      input[uiIndex] = static_cast<xiiUInt8>(uiIndex * 17);
    }
    // A native send may complete fully or partially; drain and resume using its actual byte count.
    const auto first = client.Send(input);
    if (!Check(first.m_Error))
    {
      return;
    }
    XII_TEST_BOOL(first.m_uiBytesTransferred > 0 && first.m_uiBytesTransferred <= input.GetCount());
    xiiUInt32     uiReceived = 0;
    const xiiTime deadline   = xiiTime::Now() + TestTimeout;
    while (uiReceived < first.m_uiBytesTransferred && xiiTime::Now() < deadline)
    {
      const auto result = server.Receive(output.GetArrayPtr().GetSubArray(uiReceived, first.m_uiBytesTransferred - uiReceived));
      if (result.Succeeded())
      {
        uiReceived += result.m_uiBytesTransferred;
      }
      else if (result.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        Check(result.m_Error);
        return;
      }
    }
    XII_TEST_INT(uiReceived, first.m_uiBytesTransferred);
    TransferAll(client, server, input.GetArrayPtr().GetSubArray(first.m_uiBytesTransferred), output.GetArrayPtr().GetSubArray(uiReceived));
    XII_TEST_BOOL(input.GetArrayPtr() == output.GetArrayPtr());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Vector I/O, empty buffers, graceful half-close and SIGPIPE-safe sends")
  {
    xiiTcpSocket client;
    xiiTcpSocket server;
    if (!Check(MakePair(client, server)))
    {
      return;
    }
    const xiiUInt8                    uiFirst[]     = {1, 2, 3};
    const xiiUInt8                    uiSecond[]    = {4, 5};
    const xiiArrayPtr<const xiiUInt8> sendBuffers[] = {uiFirst, {}, uiSecond};
    auto                              sent          = client.SendGather(xiiMakeArrayPtr(sendBuffers));
    Check(sent.m_Error);
    XII_TEST_INT(sent.m_uiBytesTransferred, 5);
    Check(server.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
    xiiUInt8      uiReceived[5] = {};
    xiiUInt32     uiRead        = 0;
    const xiiTime deadline      = xiiTime::Now() + TestTimeout;
    while (uiRead < 5 && xiiTime::Now() < deadline)
    {
      const xiiArrayPtr<xiiUInt8> receiveBuffers[] = {
        uiRead < 2 ? xiiArrayPtr<xiiUInt8>(uiReceived + uiRead, 2 - uiRead) : xiiArrayPtr<xiiUInt8>(),
        {},
        {uiReceived + (uiRead < 2 ? 2 : uiRead), uiRead < 2 ? 3U : 5 - uiRead}};
      const auto result = server.ReceiveScatter(xiiMakeArrayPtr(receiveBuffers));
      if (result.Succeeded())
      {
        uiRead += result.m_uiBytesTransferred;
      }
      else if (result.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock)
      {
        Check(server.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      }
      else if (result.m_Error.m_Code != xiiNetworkErrorCode::Interrupted)
      {
        Check(result.m_Error);
        return;
      }
    }
    XII_TEST_INT(uiRead, 5);
    for (xiiUInt32 uiIndex = 0; uiIndex < 5; ++uiIndex)
    {
      XII_TEST_INT(uiReceived[uiIndex], uiIndex + 1);
    }
    XII_TEST_BOOL(client.Send(xiiArrayPtr<const xiiUInt8>()).Succeeded());
    XII_TEST_BOOL(server.Receive(xiiArrayPtr<xiiUInt8>()).Succeeded());
    Check(client.Shutdown(xiiNetworkShutdown::Send));
    Check(server.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
    const auto eof = server.Receive(uiReceived);
    XII_TEST_BOOL(eof.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed);
    TransferAll(server, client, uiSecond, xiiArrayPtr<xiiUInt8>(uiReceived, 2));
    const auto shutdown = server.Shutdown(xiiNetworkShutdown::Both);
    // BSD may already consider the fully half-closed connection disconnected.
    XII_TEST_BOOL(shutdown.Succeeded() || shutdown.m_Code == xiiNetworkErrorCode::NotConnected);
    Check(server.Close());
    // A write after local send-shutdown reports an error without terminating the process.
    const auto closedSend = client.Send(uiFirst);
    XII_TEST_BOOL(closedSend.Failed());
    Check(client.Close());
    Check(client.Close());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Blocking connection and socket options")
  {
    xiiTcpListener     listener;
    xiiNetworkEndpoint endpoint;
    if (!Check(OpenListener(listener, endpoint)))
    {
      return;
    }
    xiiTcpSocket client;
    if (!Check(client.Open(xiiNetworkAddressFamily::Ipv4, false)))
    {
      return;
    }
    Check(client.SetSendTimeout(TestTimeout));
    Check(client.SetReceiveTimeout(TestTimeout));
    Check(client.SetNoDelay(true));
    Check(client.SetKeepAlive(true));
    const auto timing = client.SetKeepAliveTiming(xiiTime::MakeFromSeconds(10), xiiTime::MakeFromSeconds(2), 3);
    XII_TEST_BOOL(timing.Succeeded() || timing.m_Code == xiiNetworkErrorCode::UnsupportedOperation);
    Check(client.SetLinger(true));
    Check(client.SetLinger(false));
    Check(client.SetSendBufferSize(8192));
    Check(client.SetReceiveBufferSize(8192));
    xiiUInt32 uiSend    = 0;
    xiiUInt32 uiReceive = 0;
    Check(client.GetBufferSizes(uiSend, uiReceive));
    XII_TEST_BOOL(uiSend >= 8192 && uiReceive >= 8192);
    // Loopback listener is already listening, so the blocking connect has no external dependency.
    if (!Check(client.Connect(endpoint)) || !Check(listener.Wait(xiiNetworkSocketEvent::Readable, TestTimeout)))
    {
      return;
    }
    xiiTcpSocket server;
    Check(listener.Accept(server, nullptr, false));
    Check(server.SetReceiveTimeout(TestTimeout));
    Check(server.SetSendTimeout(TestTimeout));
    XII_TEST_BOOL(!server.IsNonBlocking());
    const xiiUInt8 uiInput[] = {42};
    xiiUInt8       uiOutput[1];
    TransferAll(client, server, uiInput, uiOutput);
    Check(client.SetNonBlocking(true));
    XII_TEST_BOOL(client.IsNonBlocking());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IPv6 loopback")
  {
    if (CanUseIpv6())
    {
      xiiTcpSocket client;
      xiiTcpSocket server;
      if (Check(MakePair(client, server, xiiNetworkAddressFamily::Ipv6)))
      {
        const xiiUInt8 uiInput[] = {6, 6, 6};
        xiiUInt8       uiOutput[3];
        TransferAll(client, server, uiInput, uiOutput);
        xiiNetworkEndpoint peer;
        Check(server.GetPeerEndpoint(peer));
        XII_TEST_BOOL(peer.GetAddress() == xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv6));
      }
    }
  }
}

XII_CREATE_SIMPLE_TEST(Network, Udp)
{
  for (const auto family : {xiiNetworkAddressFamily::Ipv4, xiiNetworkAddressFamily::Ipv6})
  {
    if (family == xiiNetworkAddressFamily::Ipv6 && !CanUseIpv6())
    {
      continue;
    }
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Datagrams, boundaries, truncation and connected UDP")
    {
      xiiUdpSocket sender;
      xiiUdpSocket receiver;
      if (!Check(sender.Open(family)) || !Check(receiver.Open(family)) || !Check(sender.Bind(Loopback(family))) || !Check(receiver.Bind(Loopback(family))))
      {
        return;
      }
      xiiNetworkEndpoint destination;
      xiiNetworkEndpoint source;
      Check(receiver.GetLocalEndpoint(destination));
      Check(sender.GetLocalEndpoint(source));
      xiiNetworkEndpoint receivedSource;
      xiiUInt8           uiBuffer[2048];
      XII_TEST_BOOL(receiver.ReceiveFrom(uiBuffer, receivedSource).m_Error.m_Code == xiiNetworkErrorCode::WouldBlock);
      const xiiUInt32 uiSizes[] = {0, 1, 128, 1024, 2048};
      for (xiiUInt32 uiSize : uiSizes)
      {
        for (xiiUInt32 uiIndex = 0; uiIndex < uiSize; ++uiIndex)
        {
          uiBuffer[uiIndex] = static_cast<xiiUInt8>(uiIndex);
        }
        const auto sent = sender.SendTo(destination, xiiArrayPtr<const xiiUInt8>(uiBuffer, uiSize));
        Check(sent.m_Error);
        XII_TEST_INT(sent.m_uiBytesTransferred, uiSize);
        Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
        const auto received = receiver.ReceiveFrom(uiBuffer, receivedSource);
        Check(received.m_Error);
        XII_TEST_INT(received.m_uiBytesTransferred, uiSize);
        XII_TEST_BOOL(receivedSource == source);
        for (xiiUInt32 uiIndex = 0; uiIndex < uiSize; ++uiIndex)
        {
          XII_TEST_INT(uiBuffer[uiIndex], static_cast<xiiUInt8>(uiIndex));
        }
      }
      const xiiUInt8                    uiFirst[]     = {11, 12, 13};
      const xiiUInt8                    uiSecond[]    = {21, 22};
      const xiiArrayPtr<const xiiUInt8> sendBuffers[] = {uiFirst, {}, uiSecond};
      Check(sender.SendToGather(destination, xiiMakeArrayPtr(sendBuffers)).m_Error);
      Check(sender.SendTo(destination, uiSecond).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      xiiUInt8                    uiLeft[2];
      xiiUInt8                    uiRight[3];
      const xiiArrayPtr<xiiUInt8> receiveBuffers[] = {uiLeft, {}, uiRight};
      auto                        received         = receiver.ReceiveFromScatter(xiiMakeArrayPtr(receiveBuffers), receivedSource);
      Check(received.m_Error);
      XII_TEST_INT(received.m_uiBytesTransferred, 5);
      XII_TEST_INT(uiLeft[0], 11);
      XII_TEST_INT(uiRight[0], 13);
      XII_TEST_INT(uiRight[2], 22);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(uiBuffer, receivedSource);
      Check(received.m_Error);
      XII_TEST_INT(received.m_uiBytesTransferred, 2);
      XII_TEST_INT(uiBuffer[0], 21);
      Check(sender.SendTo(destination, xiiArrayPtr<const xiiUInt8>(uiBuffer, 100)).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(xiiArrayPtr<xiiUInt8>(uiBuffer, 3), receivedSource);
      XII_TEST_BOOL(received.m_Error.m_Code == xiiNetworkErrorCode::Truncated);
      XII_TEST_INT(received.m_uiBytesTransferred, 3);
      XII_TEST_BOOL(receivedSource == source);
      XII_TEST_BOOL(receiver.ReceiveFrom(uiBuffer, receivedSource).m_Error.m_Code == xiiNetworkErrorCode::WouldBlock);
      // An empty receive consumes an empty datagram successfully, and a nonempty datagram as truncated.
      Check(sender.SendTo(destination, xiiArrayPtr<const xiiUInt8>()).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(xiiArrayPtr<xiiUInt8>(), receivedSource);
      Check(received.m_Error);
      XII_TEST_INT(received.m_uiBytesTransferred, 0);
      Check(sender.SendTo(destination, xiiArrayPtr<const xiiUInt8>(uiFirst, 1)).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(xiiArrayPtr<xiiUInt8>(), receivedSource);
      XII_TEST_BOOL(received.m_Error.m_Code == xiiNetworkErrorCode::Truncated);
      XII_TEST_INT(received.m_uiBytesTransferred, 0);
      Check(sender.SendTo(destination, uiFirst).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(xiiArrayPtr<xiiUInt8>(), receivedSource);
      XII_TEST_BOOL(received.m_Error.m_Code == xiiNetworkErrorCode::Truncated);
      XII_TEST_INT(received.m_uiBytesTransferred, 0);
      Check(sender.Connect(destination));
      Check(sender.Send(uiFirst).m_Error);
      Check(receiver.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      received = receiver.ReceiveFrom(uiBuffer, receivedSource);
      Check(received.m_Error);
      XII_TEST_INT(received.m_uiBytesTransferred, 3);
      XII_TEST_BOOL(receivedSource == source);
      xiiNetworkEndpoint peer;
      Check(sender.GetPeerEndpoint(peer));
      XII_TEST_BOOL(peer == destination);
    }
  }
}

XII_CREATE_SIMPLE_TEST(Network, OwnershipAndErrors)
{
  static_assert(!std::is_copy_constructible_v<xiiTcpSocket>);
  static_assert(!std::is_copy_assignable_v<xiiUdpSocket>);
  static_assert(std::is_nothrow_move_constructible_v<xiiTcpSocket>);
  static_assert(std::is_nothrow_move_assignable_v<xiiTcpListener>);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move construction/assignment, descriptor reuse, failure cleanup and repeated close")
  {
    for (xiiUInt32 uiIteration = 0; uiIteration < 128; ++uiIteration)
    {
      xiiUdpSocket original;
      if (!Check(original.Open(xiiNetworkAddressFamily::Ipv4)) || !Check(original.Bind(Loopback(xiiNetworkAddressFamily::Ipv4))))
      {
        return;
      }
      xiiNetworkEndpoint endpoint;
      Check(original.GetLocalEndpoint(endpoint));
      xiiUdpSocket moved(std::move(original));
      XII_TEST_BOOL(!original.IsOpen());
      Check(original.Close());
      xiiUdpSocket replacement;
      Check(replacement.Open(xiiNetworkAddressFamily::Ipv4));
      replacement = std::move(moved);
      XII_TEST_BOOL(!moved.IsOpen());
      xiiNetworkEndpoint afterMove;
      Check(replacement.GetLocalEndpoint(afterMove));
      XII_TEST_BOOL(afterMove == endpoint);
      // Self-move must keep the resource alive.
      auto* pReplacement = &replacement;
      replacement        = std::move(*pReplacement);
      XII_TEST_BOOL(replacement.IsOpen());
      Check(replacement.Close());
      Check(replacement.Close());
      xiiUdpSocket reused;
      Check(reused.Open(xiiNetworkAddressFamily::Ipv4));
      Check(reused.Bind(endpoint)); // Port must have been released by the moved owner.
      Check(original.Close());      // Must not close a descriptor subsequently recycled for reused.
      Check(reused.GetLocalEndpoint(afterMove));
      XII_TEST_BOOL(afterMove == endpoint);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Closed operations, argument validation and native address-in-use errors")
  {
    xiiTcpSocket closed;
    xiiUInt8     uiByte = 0;
    XII_TEST_BOOL(closed.Send(xiiMakeArrayPtr(&uiByte, 1)).m_Error.m_Code == xiiNetworkErrorCode::NotOpen);
    XII_TEST_BOOL(closed.Receive(xiiMakeArrayPtr(&uiByte, 1)).m_Error.m_Code == xiiNetworkErrorCode::NotOpen);
    XII_TEST_BOOL(closed.Shutdown(xiiNetworkShutdown::Both).m_Code == xiiNetworkErrorCode::NotOpen);
    XII_TEST_BOOL(closed.FinishConnect().m_Code == xiiNetworkErrorCode::NotOpen);
    XII_TEST_BOOL(closed.Open(xiiNetworkAddressFamily::Unspecified).m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily);
    XII_TEST_BOOL(!closed.IsOpen());
    xiiUdpSocket first;
    xiiUdpSocket second;
    if (!Check(first.Open(xiiNetworkAddressFamily::Ipv4)) || !Check(first.Bind(Loopback(xiiNetworkAddressFamily::Ipv4))) || !Check(second.Open(xiiNetworkAddressFamily::Ipv4)))
    {
      return;
    }
    xiiNetworkEndpoint endpoint;
    Check(first.GetLocalEndpoint(endpoint));
    const auto conflict = second.Bind(endpoint);
    XII_TEST_BOOL(conflict.m_Code == xiiNetworkErrorCode::AddressInUse);
    XII_TEST_BOOL(conflict.m_Domain == xiiNetworkErrorDomain::Socket);
    XII_TEST_BOOL(conflict.m_iNativeCode != 0);
    XII_TEST_BOOL(second.IsOpen());
    Check(second.Close());
    XII_TEST_BOOL(first.Open(xiiNetworkAddressFamily::Ipv4).m_Code == xiiNetworkErrorCode::AlreadyOpen);
    XII_TEST_BOOL(first.SetSendBufferSize(0).m_Code == xiiNetworkErrorCode::InvalidArgument);
    XII_TEST_BOOL(first.SetReceiveBufferSize(0xFFFFFFFFU).m_Code == xiiNetworkErrorCode::InvalidArgument);
    XII_TEST_BOOL(first.SetSendTimeout(xiiTime::MakeFromSeconds(-1)).m_Code == xiiNetworkErrorCode::InvalidArgument);
    XII_TEST_BOOL(first.Wait(xiiNetworkSocketEvent::Readable, xiiTime::MakeZero()).m_Code == xiiNetworkErrorCode::TimedOut);
    XII_TEST_BOOL(first.Send(xiiMakeArrayPtr(&uiByte, 1)).m_Error.m_Code == xiiNetworkErrorCode::NotConnected);
    const xiiNetworkEndpoint ipv6(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv6), 80);
    XII_TEST_BOOL(first.SendTo(ipv6, xiiMakeArrayPtr(&uiByte, 1)).m_Error.m_Code == xiiNetworkErrorCode::UnsupportedAddressFamily);
    xiiDynamicArray<xiiUInt8> oversized;
    oversized.SetCount(65508);
    XII_TEST_BOOL(first.SendTo(endpoint, oversized).m_Error.m_Code == xiiNetworkErrorCode::MessageTooLarge);
    const xiiArrayPtr<const xiiUInt8> tooMany[65] = {};
    XII_TEST_BOOL(first.SendToGather(endpoint, xiiMakeArrayPtr(tooMany)).m_Error.m_Code == xiiNetworkErrorCode::InvalidArgument);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nonblocking connection refusal and retained completion error")
  {
    // Bind the client while the target is reserved so it cannot select the target as its source port.
    // Release the target before connecting: BSD may leave SYNs to bound non-listeners pending.
    xiiTcpSocket reservation;
    if (!Check(reservation.Open(xiiNetworkAddressFamily::Ipv4)) || !Check(reservation.Bind(Loopback(xiiNetworkAddressFamily::Ipv4))))
    {
      return;
    }
    xiiNetworkEndpoint endpoint;
    Check(reservation.GetLocalEndpoint(endpoint));
    xiiTcpSocket client;
    Check(client.Open(xiiNetworkAddressFamily::Ipv4));
    Check(client.Bind(Loopback(xiiNetworkAddressFamily::Ipv4)));
    Check(reservation.Close());
    auto error = client.Connect(endpoint);
    if (error.m_Code == xiiNetworkErrorCode::InProgress)
    {
      if (!Check(client.Wait(xiiNetworkSocketEvent::Writable, TestTimeout)))
      {
        return;
      }
      error = client.FinishConnect();
    }
    XII_TEST_BOOL(error.m_Code == xiiNetworkErrorCode::ConnectionRefused);
    XII_TEST_BOOL(error.m_iNativeCode != 0);
    XII_TEST_BOOL(client.GetState() == xiiNetworkSocketState::Failed);
    const auto repeated = client.FinishConnect();
    XII_TEST_BOOL(repeated.m_Code == error.m_Code);
    XII_TEST_INT(repeated.m_iNativeCode, error.m_iNativeCode);
    Check(client.Close());
  }
}

XII_CREATE_SIMPLE_TEST(Network, MulticastConfiguration)
{
  xiiUdpSocket socket;
  if (!Check(socket.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  Check(socket.SetBroadcast(true));
  Check(socket.SetBroadcast(false));
  Check(socket.SetMulticastHopLimit(1));
  Check(socket.SetMulticastLoopback(true));
  const auto loopback = xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4);
  Check(socket.SetMulticastInterface(loopback));
  xiiNetworkAddress group;
  Check(xiiNetworkAddress::Parse("239.255.42.99", group));
  const auto joined = socket.JoinMulticast(group, loopback);
  if (joined.Succeeded())
  {
    Check(socket.LeaveMulticast(group, loopback));
    // Close also drops active memberships without maintaining a heap-allocated membership registry.
    Check(socket.JoinMulticast(group, loopback));
    if (Check(socket.Bind(xiiNetworkEndpoint(xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv4), 0))))
    {
      xiiNetworkEndpoint local;
      Check(socket.GetLocalEndpoint(local));
      const xiiUInt8 uiPayload[] = {7, 8, 9};
      const auto     sent        = socket.SendTo(xiiNetworkEndpoint(group, local.GetPort()), uiPayload);
      if (Check(sent.m_Error))
      {
        const auto ready = socket.Wait(xiiNetworkSocketEvent::Readable, xiiTime::MakeFromSeconds(1));
        if (ready.Succeeded())
        {
          xiiUInt8           uiReceived[3];
          xiiNetworkEndpoint sender;
          const auto         received = socket.ReceiveFrom(uiReceived, sender);
          Check(received.m_Error);
          XII_TEST_INT(received.m_uiBytesTransferred, 3);
          XII_TEST_BOOL(xiiMakeArrayPtr(uiReceived) == xiiMakeArrayPtr(uiPayload));
          XII_TEST_BOOL(sender.GetAddress().IsLoopback());
        }
        else if (ready.m_Code == xiiNetworkErrorCode::TimedOut)
        {
          xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped multicast delivery: loopback routing policy did not deliver the joined group.");
        }
        else
        {
          Check(ready);
        }
      }
    }
  }
  else
  {
    XII_TEST_BOOL(joined.m_Code == xiiNetworkErrorCode::UnsupportedOperation || joined.m_Code == xiiNetworkErrorCode::AddressNotAvailable || joined.m_Code == xiiNetworkErrorCode::NetworkUnreachable || joined.m_Code == xiiNetworkErrorCode::PermissionDenied);
    xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped loopback multicast membership: %s (native %d)", joined.GetCodeName(), joined.m_iNativeCode);
  }
  Check(socket.Close());
}

XII_CREATE_SIMPLE_TEST(Network, ResourceLifetime)
{
  // Keep the native runtime initialized throughout the census (important on Windows).
  xiiUdpSocket runtimeOwner;
  if (!Check(runtimeOwner.Open(xiiNetworkAddressFamily::Ipv4)))
  {
    return;
  }
  xiiDynamicArray<xiiNetworkEndpoint> endpoints;
  Check(xiiNetworkResolver::Resolve("127.0.0.1", "0", endpoints));
  xiiUInt32 uiBefore = xiiNetworkTestUtilities::CountOpenResources();
  if (!XII_TEST_BOOL(uiBefore != 0))
  {
    return;
  }
  for (xiiUInt32 uiPass = 0; uiPass < 3; ++uiPass)
  {
    for (xiiUInt32 uiIteration = 0; uiIteration < 32; ++uiIteration)
    {
      xiiTcpSocket client;
      xiiTcpSocket server;
      if (!Check(MakePair(client, server)))
      {
        return;
      }
      xiiTcpSocket moved(std::move(client));
      Check(server.SetLinger(true));
      Check(server.Close());
      Check(moved.Wait(xiiNetworkSocketEvent::Readable, TestTimeout));
      xiiUInt8   uiBuffer[8] = {};
      const auto reset       = moved.Receive(uiBuffer);
      XII_TEST_BOOL(reset.m_Error.m_Code == xiiNetworkErrorCode::ConnectionReset);
      // No SIGPIPE on this path, including a write after observing a native reset.
      XII_TEST_BOOL(moved.Send(uiBuffer).Failed());
      Check(xiiNetworkResolver::Resolve("127.0.0.1", "0", endpoints));
      xiiUdpSocket first;
      xiiUdpSocket failedBind;
      Check(first.Open(xiiNetworkAddressFamily::Ipv4));
      Check(first.Bind(Loopback(xiiNetworkAddressFamily::Ipv4)));
      xiiNetworkEndpoint endpoint;
      Check(first.GetLocalEndpoint(endpoint));
      Check(failedBind.Open(xiiNetworkAddressFamily::Ipv4));
      XII_TEST_BOOL(failedBind.Bind(endpoint).Failed());
    }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    if (uiPass == 0)
    {
      // Winsock may initialize persistent provider resources on the first reset/cleanup path.
      // Two further identical batches must not grow the process handle count.
      uiBefore = xiiNetworkTestUtilities::CountOpenResources();
      continue;
    }
#endif
    XII_TEST_INT(xiiNetworkTestUtilities::CountOpenResources(), uiBefore);
  }
}

XII_CREATE_SIMPLE_TEST(Network, BackpressureAndDualStack)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Would-block progress and pending connection preconditions")
  {
    xiiTcpSocket client;
    xiiTcpSocket server;
    if (!Check(MakePair(client, server)))
    {
      return;
    }
    Check(client.SetSendBufferSize(4096));
    xiiUInt8      uiData[16384] = {};
    bool          bBlocked      = false;
    const xiiTime deadline      = xiiTime::Now() + TestTimeout;
    while (xiiTime::Now() < deadline)
    {
      const auto sent = client.Send(uiData);
      if (sent.m_Error.m_Code == xiiNetworkErrorCode::WouldBlock)
      {
        bBlocked = true;
        XII_TEST_BOOL(sent.m_Error.m_iNativeCode != 0);
        XII_TEST_INT(sent.m_uiBytesTransferred, 0);
        break;
      }
      if (!Check(sent.m_Error))
      {
        return;
      }
    }
    XII_TEST_BOOL(bBlocked);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit IPv6-only and dual-stack configuration")
  {
    if (!CanUseIpv6())
    {
      return;
    }
    xiiTcpListener listener;
    if (!Check(listener.Open(xiiNetworkAddressFamily::Ipv6)))
    {
      return;
    }
    Check(listener.SetIpv6Only(true));
    const auto dualStack = listener.SetIpv6Only(false);
    if (dualStack.m_Code == xiiNetworkErrorCode::UnsupportedOperation)
    {
      xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped dual-stack listener: unsupported by platform.");
      return;
    }
    if (!Check(dualStack) || !Check(listener.Bind(xiiNetworkEndpoint(xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv6), 0))) || !Check(listener.Listen()))
    {
      return;
    }
    xiiNetworkEndpoint local;
    Check(listener.GetLocalEndpoint(local));
    xiiTcpSocket client;
    if (!Check(ConnectClient(client, xiiNetworkEndpoint(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), local.GetPort()))) || !Check(listener.Wait(xiiNetworkSocketEvent::Readable, TestTimeout)))
    {
      return;
    }
    xiiTcpSocket       server;
    xiiNetworkEndpoint peer;
    Check(listener.Accept(server, &peer));
    XII_TEST_BOOL(peer.GetAddress().GetFamily() == xiiNetworkAddressFamily::Ipv6);
    xiiStringBuilder sPeer;
    peer.GetAddress().ToString(sPeer);
    XII_TEST_STRING(sPeer, "::ffff:7f00:1");
    const xiiUInt8 uiInput[] = {4, 6};
    xiiUInt8       uiOutput[2];
    TransferAll(client, server, uiInput, uiOutput);
  }
}
