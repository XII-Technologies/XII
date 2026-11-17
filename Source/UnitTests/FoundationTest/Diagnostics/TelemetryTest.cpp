/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Diagnostics/TelemetryDiscovery.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Network/Reflection/NetworkSchema.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <FoundationTest/Network/Implementation/NetworkSecurityTestUtilities.h>
#include <algorithm>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#else
#  include <time.h>
#endif

XII_CREATE_SIMPLE_TEST_GROUP(Diagnostics);
XII_CREATE_SIMPLE_TEST_GROUP(DiagnosticsBenchmark);

namespace
{
  constexpr xiiUInt64 SourceId          = 0x53544154532F5632ULL;
  constexpr xiiUInt64 ProtectedSourceId = 0x434F4E54524F4C32ULL;

  xiiTelemetryConfiguration LocalConfiguration()
  {
    xiiTelemetryConfiguration configuration;
    configuration.m_Network.m_Transport            = xiiNetworkTransportType::Local;
    configuration.m_Network.m_Authentication       = xiiNetworkAuthenticationPolicy::LocalUser;
    configuration.m_Network.m_uiMaximumConnections = 4;
    configuration.m_sApplication                   = "Telemetry test";
    return configuration;
  }

  bool PumpUntil(xiiTelemetryService& service, xiiTelemetryClient& first, xiiTelemetryClient& second, xiiDelegate<bool()> done)
  {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    while (xiiTime::Now() < deadline)
    {
      service.Update();
      first.Update();
      second.Update();
      if (done())
        return true;
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    return false;
  }

  xiiTelemetrySourceInfo Source(xiiUInt64 id)
  {
    xiiTelemetrySourceInfo source;
    source.m_uiId                          = id;
    source.m_sName                         = "Counter";
    source.m_sCategory                     = "Tests";
    source.m_uiMinimumIntervalMilliseconds = 1;
    source.m_bSnapshot                     = true;
    source.m_bLatestOnly                   = true;
    return source;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Diagnostics, IndependentSubscriptions)
{
  auto                config = LocalConfiguration();
  xiiTelemetryService service;
  xiiUInt32           collections = 0;
  auto                collect     = [&](xiiTelemetryService& owner, xiiUInt64 id) {
    ++collections;
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    sample.m_Fields.Insert("value", collections);
    XII_TEST_RESULT(owner.Publish(std::move(sample)));
  };
  XII_TEST_RESULT(service.RegisterSource(Source(SourceId), collect));
  auto protectedSource           = Source(ProtectedSourceId);
  protectedSource.m_uiPermission = static_cast<xiiUInt8>(xiiTelemetryPermission::Configure);
  XII_TEST_RESULT(service.RegisterSource(protectedSource, collect));

  xiiStringBuilder name;
  name.SetFormat("diag-{}", xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) ||
      !XII_TEST_BOOL(service.Start(config).Succeeded()) || !XII_TEST_BOOL(service.Listen(endpoint).Succeeded()))
    return;
  xiiTelemetryClient first, second;
  if (!XII_TEST_BOOL(first.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(second.Connect(config, endpoint).Succeeded()) ||
      !XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return first.IsReady() && second.IsReady(); })))
    return;
  XII_TEST_INT(collections, 0);
  XII_TEST_INT(first.GetServerInfo().m_Sources.GetCount(), 1);
  XII_TEST_BOOL(first.GetServerInfo().m_Instance == service.GetInstanceId());

  xiiUInt32 firstSamples = 0, secondSamples = 0;
  xiiUInt64 firstSequence = 0, secondSequence = 0;
  first.SetSampleHandler([&](const xiiTelemetrySample& sample) {
    ++firstSamples;
    XII_TEST_BOOL(sample.m_uiGeneration == 11 && sample.m_uiSequence > firstSequence);
    firstSequence = sample.m_uiSequence;
  });
  second.SetSampleHandler([&](const xiiTelemetrySample& sample) {
    ++secondSamples;
    XII_TEST_BOOL(sample.m_uiGeneration == 22 && sample.m_uiSequence > secondSequence);
    secondSequence = sample.m_uiSequence;
  });

  xiiTelemetrySubscription subscription;
  subscription.m_uiRequestId            = 1;
  subscription.m_uiSource               = SourceId;
  subscription.m_uiGeneration           = 11;
  subscription.m_uiIntervalMilliseconds = 1;
  XII_TEST_BOOL(first.Subscribe(subscription).Succeeded());
  if (!XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return firstSamples >= 2; })))
    return;
  XII_TEST_INT(secondSamples, 0);
  subscription.m_uiGeneration           = 22;
  subscription.m_uiIntervalMilliseconds = 50;
  XII_TEST_BOOL(second.Subscribe(subscription).Succeeded());
  if (!XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return secondSamples >= 2; })))
    return;
  XII_TEST_BOOL(firstSamples > secondSamples);
  XII_TEST_INT(service.GetStatistics().m_uiSubscriptions, 2);

  bool rejected = false;
  second.SetSubscriptionHandler([&](const xiiTelemetrySubscriptionResult& response) {
    rejected |= response.m_uiSource == ProtectedSourceId && response.m_uiError == static_cast<xiiUInt8>(xiiNetworkDataErrorCode::Unauthorized);
  });
  subscription.m_uiSource    = ProtectedSourceId;
  subscription.m_uiRequestId = 2;
  XII_TEST_BOOL(second.Subscribe(subscription).Succeeded());
  XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return rejected; }));

  first.Shutdown();
  const auto previousSecond = secondSamples;
  XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return secondSamples > previousSecond && service.GetStatistics().m_uiClients == 1; }));
  subscription.m_uiSource    = SourceId;
  subscription.m_uiRequestId = 3;
  subscription.m_bEnabled    = false;
  XII_TEST_BOOL(second.Subscribe(subscription).Succeeded());
  XII_TEST_BOOL(PumpUntil(service, first, second, [&]() { return service.GetStatistics().m_uiSubscriptions == 0; }));
}

XII_CREATE_SIMPLE_TEST(Diagnostics, OfflineCaptureAndOwnership)
{
  xiiTelemetryService service;
  XII_TEST_RESULT(service.RegisterSource(Source(SourceId), [](xiiTelemetryService& owner, xiiUInt64 id) {
    xiiTelemetrySample sample;
    sample.m_uiSource = id;
    sample.m_Fields.Insert("counter", 42);
    XII_TEST_RESULT(owner.Publish(std::move(sample)));
  }));
  xiiUInt32 samples = 0;
  XII_TEST_RESULT(service.Capture(SourceId, [&](const xiiTelemetrySample& sample) {
    ++samples;
    const auto* value = sample.m_Fields.GetValue("counter");
    XII_TEST_BOOL(value != nullptr && value->Get<xiiInt32>() == 42);
  }));
  XII_TEST_INT(samples, 1);
  XII_TEST_INT(service.GetNetworkStatistics().m_uiActiveConnections, 0);

  xiiTelemetrySample rejected;
  rejected.m_uiSource = SourceId;
  rejected.m_Data.SetCount(128, 7);
  XII_TEST_BOOL(service.Publish(std::move(rejected)).Failed());
  XII_TEST_INT(rejected.m_Data.GetCount(), 128);
  service.UnregisterSource(SourceId);
  XII_TEST_BOOL(service.Capture(SourceId, [](const xiiTelemetrySample&) {}).Failed());
}

XII_CREATE_SIMPLE_TEST(Diagnostics, CommandsAuthorizationAndCorrelation)
{
  xiiCVarInt variable("Diagnostics.Test.Control", 1, xiiCVarFlags::None, "Diagnostics authorization fixture.");
  auto       config      = LocalConfiguration();
  config.m_uiPermissions = 15;
  xiiTelemetryService service;
  xiiUInt32           authorized = 0, commands = 0;
  service.SetConsoleHandler([&](xiiStringView text, bool complete) {
    XII_TEST_BOOL(xiiThreadUtils::IsMainThread());
    ++commands;
    xiiTelemetryCommandResponse response;
    response.m_sOutput = text;
    return response;
  });
  if (!XII_TEST_BOOL(service.Start(config, [&](const xiiNetworkSessionInfo&) { return xiiUInt8(++authorized == 1 ? 3 : 15); }).Succeeded()))
    return;
  xiiStringBuilder name;
  name.SetFormat("diag-rpc-{}", xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) || !XII_TEST_BOOL(service.Listen(endpoint).Succeeded()))
    return;
  xiiTelemetryClient reader, writer;
  if (!XII_TEST_BOOL(reader.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] { return reader.IsReady(); })) ||
      !XII_TEST_BOOL(writer.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] { return writer.IsReady(); })))
    return;
  xiiNetworkRpcRequest denied, change, command, completion;
  XII_TEST_BOOL(reader.SetCVar("Diagnostics.Test.Control", 99, denied).Succeeded());
  xiiNetworkRpcResult         result;
  xiiTelemetryCommandResponse response;
  XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] { return reader.TryGetCommandResult(result, response); }));
  XII_TEST_BOOL(result.m_Request == denied && result.m_Error.m_Code == xiiNetworkDataErrorCode::Unauthorized);
  XII_TEST_INT(variable.GetValue(), 1);
  XII_TEST_BOOL(writer.SetCVar("Diagnostics.Test.Control", 42, change).Succeeded());
  XII_TEST_BOOL(writer.Console("execute", false, command).Succeeded());
  XII_TEST_BOOL(writer.Console("complete", true, completion).Succeeded());
  xiiUInt32 responses = 0;
  XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] {
    while (writer.TryGetCommandResult(result, response))
    {
      XII_TEST_BOOL(result.m_Error.Succeeded() && response.m_uiError == 0);
      if (result.m_Request == command)
        XII_TEST_STRING(response.m_sOutput, "execute");
      else if (result.m_Request == completion)
        XII_TEST_STRING(response.m_sOutput, "complete");
      else
        XII_TEST_BOOL(result.m_Request == change);
      ++responses;
    }
    return responses == 3;
  }));
  XII_TEST_INT(variable.GetValue(), 42);
  XII_TEST_INT(commands, 2);
  xiiNetworkRpcRequest invalid;
  XII_TEST_BOOL(writer.SetCVar("Diagnostics.Test.Control", xiiString("wrong type"), invalid).Succeeded());
  XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] { return writer.TryGetCommandResult(result, response); }));
  XII_TEST_BOOL(result.m_Request == invalid && result.m_Error.Succeeded() && response.m_uiError == static_cast<xiiUInt8>(xiiNetworkDataErrorCode::InvalidArgument));
  XII_TEST_INT(variable.GetValue(), 42);
  xiiNetworkRpcRequest cancelled;
  XII_TEST_BOOL(writer.Console("cancel", false, cancelled).Succeeded());
  XII_TEST_BOOL(writer.CancelCommand(cancelled).Succeeded());
  XII_TEST_BOOL(writer.TryGetCommandResult(result, response));
  XII_TEST_BOOL(result.m_Request == cancelled && result.m_Error.m_Code == xiiNetworkDataErrorCode::Cancelled);
  xiiNetworkRpcRequest disconnected;
  XII_TEST_BOOL(writer.Console("shutdown", false, disconnected).Succeeded());
  service.Shutdown();
  XII_TEST_BOOL(PumpUntil(service, reader, writer, [&] { return writer.TryGetCommandResult(result, response); }));
  XII_TEST_BOOL(result.m_Request == disconnected && result.m_Error.m_Code == xiiNetworkDataErrorCode::Disconnected);
}

XII_CREATE_SIMPLE_TEST(Diagnostics, CapturePreservesLiveQueue)
{
  xiiTelemetryService service;
  auto                info = Source(SourceId);
  info.m_bLatestOnly       = false;
  // Keep the scheduled collector idle while the test explicitly captures and publishes.
  info.m_uiMinimumIntervalMilliseconds = 60000;
  bool bCapture                        = false;
  XII_TEST_RESULT(service.RegisterSource(info, [&](xiiTelemetryService& owner, xiiUInt64 id) {
    if (!bCapture)
      return;
    xiiTelemetrySample sample;
    sample.m_uiSource = id;
    sample.m_Data.PushBack(2);
    XII_TEST_RESULT(owner.Publish(std::move(sample)));
  }));

  const auto config = LocalConfiguration();
  if (!XII_TEST_BOOL(service.StartLocal("Capture ownership test").Succeeded()))
    return;
  xiiNetworkLocalEndpoint endpoint;
  xiiTelemetryClient      client, unused;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(service.GetLocalEndpointName(), endpoint).Succeeded()) ||
      !XII_TEST_BOOL(client.Connect(config, endpoint).Succeeded()) ||
      !XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return client.IsReady(); })))
    return;
  xiiTelemetrySubscription request;
  request.m_uiSource     = SourceId;
  request.m_uiGeneration = request.m_uiRequestId = 1;
  request.m_uiIntervalMilliseconds               = 60000;
  if (!XII_TEST_BOOL(client.Subscribe(request).Succeeded()) ||
      !XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return service.IsCollecting(SourceId); })))
    return;

  xiiTelemetrySample queued;
  queued.m_uiSource = SourceId;
  queued.m_Data.PushBack(1);
  XII_TEST_RESULT(service.Publish(std::move(queued)));
  xiiUInt32 captured = 0;
  bCapture           = true;
  XII_TEST_RESULT(service.Capture(SourceId, [&](const xiiTelemetrySample& sample) {
    ++captured;
    if (XII_TEST_INT(sample.m_Data.GetCount(), 1))
      XII_TEST_INT(sample.m_Data[0], 2);
  }));
  bCapture = false;
  XII_TEST_INT(captured, 1);
  XII_TEST_INT(service.GetStatistics().m_uiQueuedSamples, 2);
}

XII_CREATE_SIMPLE_TEST(Diagnostics, BoundedAdmissionAndSourceLifetime)
{
  auto config                     = LocalConfiguration();
  config.m_uiMaximumQueuedSamples = 2;
  xiiTelemetryService service;
  xiiUInt32           starts = 0, stops = 0;
  auto                info = Source(SourceId);
  info.m_bLatestOnly       = false;
  XII_TEST_RESULT(service.RegisterSource(info, {}, [&](bool active) { active ? ++starts : ++stops; }));
  xiiStringBuilder name;
  name.SetFormat("diag-limit-{}", xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) || !XII_TEST_BOOL(service.Start(config).Succeeded()) || !XII_TEST_BOOL(service.Listen(endpoint).Succeeded()))
    return;
  xiiTelemetryClient client, unused;
  if (!XII_TEST_BOOL(client.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return client.IsReady(); })))
    return;
  xiiTelemetrySubscription subscription;
  subscription.m_uiSource               = SourceId;
  subscription.m_uiGeneration           = 31;
  subscription.m_uiRequestId            = 1;
  subscription.m_uiIntervalMilliseconds = 1;
  XII_TEST_BOOL(client.Subscribe(subscription).Succeeded());
  XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return service.IsCollecting(SourceId); }));
  XII_TEST_INT(starts, 1);
  XII_TEST_BOOL(client.Subscribe(subscription).m_Code == xiiNetworkDataErrorCode::DuplicateRequest);
  auto stale           = subscription;
  stale.m_uiRequestId  = 2;
  stale.m_uiGeneration = 30;
  XII_TEST_BOOL(client.Subscribe(stale).m_Code == xiiNetworkDataErrorCode::DuplicateRequest);
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource = SourceId;
    sample.m_Data.SetCount(128, 7);
    auto& fields = sample.m_Records.ExpandAndGetRef().m_Fields;
    fields.Insert("value", i);
    const auto admitted = service.Publish(std::move(sample));
    XII_TEST_BOOL(admitted.Succeeded() == (i < 2));
    if (i == 2)
      XII_TEST_INT(sample.m_Data.GetCount(), 128);
  }
  xiiUInt32 received = 0;
  client.SetSampleHandler([&](const xiiTelemetrySample& sample) {
    xiiUInt32 value = 99;
    XII_TEST_INT(sample.m_Records.GetCount(), 1);
    XII_TEST_BOOL(sample.m_Records[0].Read("value", value));
    XII_TEST_INT(value, received++);
  });
  XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return received == 2; }));
  XII_TEST_BOOL(service.GetStatistics().m_uiSamplesDropped >= 1);
  service.UnregisterSource(SourceId);
  XII_TEST_BOOL(!service.IsCollecting(SourceId));
  XII_TEST_INT(stops, 1);
  XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return !client.IsSourceAvailable(SourceId); }));
}

XII_CREATE_SIMPLE_TEST(Diagnostics, DiscoveryLeaseLifetime)
{
  xiiTelemetryDiscovery registration;
  xiiTelemetryInstance  instance;
  instance.m_Instance     = xiiUuid::MakeUuid();
  instance.m_sApplication = "Offline discovery fixture";
  instance.m_sEndpoint    = "offline-discovery-fixture";
  XII_TEST_RESULT(registration.Publish(instance));
  xiiDynamicArray<xiiTelemetryInstance> instances;
  XII_TEST_RESULT(xiiTelemetryDiscovery::Enumerate(instances));
  bool found = false;
  for (const auto& entry : instances)
    found |= entry.m_Instance == instance.m_Instance;
  XII_TEST_BOOL(found);
  registration.Reset();
  XII_TEST_RESULT(xiiTelemetryDiscovery::Enumerate(instances));
  for (const auto& entry : instances)
    XII_TEST_BOOL(entry.m_Instance != instance.m_Instance);
}

XII_CREATE_SIMPLE_TEST(Diagnostics, SlowOrderedSubscriberIsIsolated)
{
  xiiTelemetryService service;
  auto                info = Source(SourceId);
  info.m_bLatestOnly       = false;
  XII_TEST_RESULT(service.RegisterSource(info));

  auto config                     = LocalConfiguration();
  config.m_uiMaximumQueuedSamples = 2;
  xiiStringBuilder name;
  name.SetFormat("diag-backpressure-{}", xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  xiiNetworkLocalEndpoint endpoint;
  xiiTelemetryClient      fast, slow;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) ||
      !XII_TEST_BOOL(service.Start(config).Succeeded()) || !XII_TEST_BOOL(service.Listen(endpoint).Succeeded()) ||
      !XII_TEST_BOOL(fast.Connect(config, endpoint).Succeeded()) || !XII_TEST_BOOL(slow.Connect(config, endpoint).Succeeded()) ||
      !XII_TEST_BOOL(PumpUntil(service, fast, slow, [&] { return fast.IsReady() && slow.IsReady(); })))
    return;

  xiiUInt32 fastSamples = 0, slowSamples = 0;
  fast.SetSampleHandler([&](const xiiTelemetrySample& sample) { XII_TEST_INT(sample.m_uiSequence, ++fastSamples); });
  slow.SetSampleHandler([&](const xiiTelemetrySample&) { ++slowSamples; });
  xiiTelemetrySubscription request;
  request.m_uiSource     = SourceId;
  request.m_uiGeneration = request.m_uiRequestId = 1;
  request.m_uiIntervalMilliseconds               = 1;
  XII_TEST_BOOL(fast.Subscribe(request).Succeeded());
  request.m_uiIntervalMilliseconds = 60000;
  XII_TEST_BOOL(slow.Subscribe(request).Succeeded());
  if (!XII_TEST_BOOL(PumpUntil(service, fast, slow, [&] { return service.GetStatistics().m_uiSubscriptions == 2; })))
    return;

  for (xiiUInt32 i = 1; i <= 4; ++i)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource = SourceId;
    sample.m_Data.PushBack(42);
    if (!XII_TEST_RESULT(service.Publish(std::move(sample))) ||
        !XII_TEST_BOOL(PumpUntil(service, fast, slow, [&] { return fastSamples == i && (i != 1 || slowSamples == 1); })))
      return;
  }
  XII_TEST_BOOL(PumpUntil(service, fast, slow, [&] { return !slow.IsReady(); }));
  XII_TEST_BOOL(fast.IsReady());
  XII_TEST_INT(slowSamples, 1);
  XII_TEST_BOOL(service.GetStatistics().m_uiBackpressureEvents > 0);
}

XII_CREATE_SIMPLE_TEST(Diagnostics, AuthenticatedRemoteTransports)
{
  NetworkSecurityTestCertificates certificates;
  if (!XII_TEST_RESULT(certificates.Initialize()))
    return;
  for (auto transport : {xiiNetworkTransportType::TlsTcp, xiiNetworkTransportType::SecureDatagram})
  {
    auto serverConfig                       = LocalConfiguration();
    serverConfig.m_Network.m_Transport      = transport;
    serverConfig.m_Network.m_Authentication = xiiNetworkAuthenticationPolicy::Certificate;
    auto clientConfig                       = serverConfig;
    serverConfig.m_Network.m_pCredentials   = XII_DEFAULT_NEW(xiiNetworkCredentials);
    clientConfig.m_Network.m_pCredentials   = XII_DEFAULT_NEW(xiiNetworkCredentials);
    if (!XII_TEST_BOOL(serverConfig.m_Network.m_pCredentials->Load(certificates.Configuration()).Succeeded()) ||
        !XII_TEST_BOOL(clientConfig.m_Network.m_pCredentials->Load(certificates.Configuration("client")).Succeeded()))
      return;
    xiiTelemetryService service;
    if (!XII_TEST_BOOL(service.Start(serverConfig).Succeeded()))
      return;
    xiiNetworkEndpoint endpoint;
    if (!XII_TEST_BOOL(service.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 0}, &endpoint).Succeeded()))
      return;
    xiiTelemetryClient       client, unused;
    xiiNetworkConnectOptions options;
    options.m_sServerName = "localhost";
    if (!XII_TEST_BOOL(client.Connect(clientConfig, endpoint, options).Succeeded()) ||
        !XII_TEST_BOOL(PumpUntil(service, client, unused, [&] { return client.IsReady(); })))
      return;
    xiiNetworkConnectionInfo info;
    XII_TEST_BOOL(client.GetConnectionInfo(info).Succeeded());
    XII_TEST_BOOL(info.m_Capabilities.m_bAuthenticated && info.m_Capabilities.m_bEncrypted);
    XII_TEST_BOOL(!info.m_sAuthenticatedIdentity.IsEmpty());
  }
  auto insecure                                  = LocalConfiguration();
  insecure.m_Network.m_Transport                 = xiiNetworkTransportType::Tcp;
  insecure.m_Network.m_bAllowInsecureDevelopment = true;
  xiiTelemetryService rejected;
  XII_TEST_BOOL(rejected.Start(insecure).m_Code == xiiNetworkErrorCode::InvalidArgument);
}

XII_CREATE_SIMPLE_TEST(DiagnosticsBenchmark, NativeLocalPublication)
{
  const auto* commandLine = xiiCommandLineUtils::GetGlobalInstance();
  if (!commandLine->GetBoolOption("-diagnosticsBenchmark"))
    return;
  const auto clients    = static_cast<xiiUInt32>(xiiMath::Clamp(commandLine->GetIntOption("-diagnosticsBenchmarkClients", 1), 1, 4));
  auto       cpuSeconds = []() -> double {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    FILETIME creation, exit, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
      return 0;
    const auto ticks = [](FILETIME value) {
      return (xiiUInt64(value.dwHighDateTime) << 32) | value.dwLowDateTime;
    };
    return (ticks(kernel) + ticks(user)) / 10000000.0;
#else
    timespec value = {};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value);
    return value.tv_sec + value.tv_nsec / 1000000000.0;
#endif
  };
  xiiTelemetryService service;
  auto                info = Source(SourceId);
  XII_TEST_RESULT(service.RegisterSource(info, [](xiiTelemetryService& owner, xiiUInt64 id) {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    sample.m_Data.SetCount(64, 0xAB);
    XII_TEST_RESULT(owner.Publish(std::move(sample)));
  }));
  info.m_uiId        = SourceId + 1;
  info.m_bLatestOnly = false;
  XII_TEST_RESULT(service.RegisterSource(info));
  auto             config = LocalConfiguration();
  xiiStringBuilder name;
  name.SetFormat("diag-bench-{}", xiiArgU(xiiTime::Now().GetMicroseconds(), 1, false, 16));
  xiiNetworkLocalEndpoint endpoint;
  if (!XII_TEST_BOOL(xiiNetworkLocalEndpoint::Make(name, endpoint).Succeeded()) || !XII_TEST_BOOL(service.Start(config).Succeeded()) || !XII_TEST_BOOL(service.Listen(endpoint).Succeeded()))
    return;
  xiiTelemetryClient peers[4];
  auto&              client = peers[0];
  auto               pump   = [&] {
    service.Update();
    for (xiiUInt32 i = 0; i < clients; ++i)
      peers[i].Update();
  };
  auto waitFor = [&](auto&& done) {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(10);
    while (xiiTime::Now() < deadline)
    {
      pump();
      if (done())
        return true;
    }
    return false;
  };
  const auto connectStarted = xiiTime::Now();
  for (xiiUInt32 i = 0; i < clients; ++i)
    if (!XII_TEST_BOOL(peers[i].Connect(config, endpoint).Succeeded()))
      return;
  if (!XII_TEST_BOOL(waitFor([&] {
        for (xiiUInt32 i = 0; i < clients; ++i)
          if (!peers[i].IsReady())
            return false;
        return true;
      })))
    return;
  const auto established = xiiTime::Now() - connectStarted;
  xiiUInt32  received[4] = {};
  for (xiiUInt32 i = 0; i < clients; ++i)
    peers[i].SetSampleHandler([&, i](const xiiTelemetrySample&) { ++received[i]; });
  auto wait = [&](xiiUInt32 target, xiiUInt32 peerCount) {
    return XII_TEST_BOOL(waitFor([&] {
      for (xiiUInt32 i = 0; i < peerCount; ++i)
        if (received[i] < target)
          return false;
      return true;
    }));
  };
  xiiTelemetrySubscription request;
  request.m_uiSource               = SourceId;
  request.m_uiIntervalMilliseconds = 1;
  request.m_bSnapshot              = true;
  double latency[128];
  for (xiiUInt32 i = 0; i < 128; ++i)
  {
    request.m_uiRequestId  = i + 1;
    request.m_uiGeneration = i + 1;
    const auto started     = xiiTime::Now();
    if (!XII_TEST_BOOL(client.Subscribe(request).Succeeded()) || !wait(i + 1, 1))
      return;
    latency[i] = (xiiTime::Now() - started).GetMicroseconds();
  }
  std::sort(std::begin(latency), std::end(latency));
  request.m_uiRequestId  = 129;
  request.m_uiGeneration = 129;
  request.m_bEnabled = request.m_bSnapshot = false;
  XII_TEST_BOOL(client.Subscribe(request).Succeeded());
  request.m_uiSource    = SourceId + 1;
  request.m_uiRequestId = request.m_uiGeneration = 1;
  request.m_bEnabled                             = true;
  for (xiiUInt32 i = 0; i < clients; ++i)
    XII_TEST_BOOL(peers[i].Subscribe(request).Succeeded());
  if (!XII_TEST_BOOL(waitFor([&] { return service.GetStatistics().m_uiSubscriptions == clients && service.IsCollecting(SourceId + 1); })))
    return;
  received[0]                  = 0;
  const auto allocationsBefore = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
  const auto cpuStarted        = cpuSeconds();
  const auto transferStarted   = xiiTime::Now();
  for (xiiUInt32 batch = 0; batch < 32; ++batch)
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      xiiTelemetrySample sample;
      sample.m_uiSource               = SourceId + 1;
      sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
      sample.m_Data.SetCount(256 * 1024, 0xAB);
      if (!XII_TEST_RESULT(service.Publish(std::move(sample))))
        return;
    }
    if (!wait((batch + 1) * 4, clients))
      return;
  }
  const auto elapsed     = xiiTime::Now() - transferStarted;
  const auto cpu         = cpuSeconds() - cpuStarted;
  const auto allocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount - allocationsBefore;
  const auto stats       = service.GetStatistics();
  const auto network     = service.GetNetworkStatistics();
  xiiTestFramework::Output(xiiTestOutput::Details,
                           "Telemetry Local: %u clients ready in %.1f us; 64-byte snapshot request p50/p99 %.1f/%.1f us; 256KiB publication %.2f MiB/s aggregate; %.1f%% process CPU; %.1f default allocations/delivery; pool committed/leased %llu/%llu, reused %llu; drops %llu; collection/serialization/publication %.2f/%.2f/%.2f ms",
                           clients, established.GetMicroseconds(), latency[63], latency[126], 32.0 * clients / elapsed.GetSeconds(), 100.0 * cpu / elapsed.GetSeconds(),
                           double(allocations) / (128 * clients), static_cast<unsigned long long>(network.m_IO.m_BufferPool.m_uiCommittedBytes),
                           static_cast<unsigned long long>(network.m_IO.m_BufferPool.m_uiLeasedBytes), static_cast<unsigned long long>(network.m_IO.m_BufferPool.m_uiReuses),
                           static_cast<unsigned long long>(stats.m_uiSamplesDropped), stats.m_CollectionTime.GetMilliseconds(), stats.m_SerializationTime.GetMilliseconds(), stats.m_PublicationTime.GetMilliseconds());
}
