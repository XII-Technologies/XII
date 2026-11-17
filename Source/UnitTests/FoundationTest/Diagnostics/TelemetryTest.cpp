/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Network/Reflection/NetworkSchema.h>
#include <Foundation/Threading/ThreadUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(Diagnostics);

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
