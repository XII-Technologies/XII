/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Diagnostics/TelemetryProtocol.h>
#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Types/Delegate.h>

struct XII_FOUNDATION_DLL xiiTelemetryConfiguration
{
  xiiTelemetryConfiguration() { m_Network.m_uiMaximumConnections = 8; }
  xiiNetworkHostConfiguration m_Network;
  xiiString                   m_sApplication;
  xiiUInt32                   m_uiMaximumQueuedSamples = 1024;
  xiiUInt64                   m_uiMaximumQueuedBytes   = 32 * 1024 * 1024;
  xiiUInt64                   m_uiMaximumClientBytes   = 32 * 1024 * 1024;
  /// Read and capture are separate permissions. Mutating permissions require explicit configuration.
  xiiUInt8 m_uiPermissions = static_cast<xiiUInt8>(xiiTelemetryPermission::Read) | static_cast<xiiUInt8>(xiiTelemetryPermission::Capture);
};

struct XII_FOUNDATION_DLL xiiTelemetryStatistics
{
  xiiUInt32 m_uiClients            = 0;
  xiiUInt32 m_uiSubscriptions      = 0;
  xiiUInt32 m_uiQueuedSamples      = 0;
  xiiUInt64 m_uiQueuedBytes        = 0;
  xiiUInt64 m_uiSamplesCollected   = 0;
  xiiUInt64 m_uiSamplesDropped     = 0;
  xiiUInt64 m_uiPublishedBytes     = 0;
  xiiUInt64 m_uiBackpressureEvents = 0;
  xiiTime   m_CollectionTime;
  xiiTime   m_PublicationTime;
};

class xiiTelemetryServiceState;

struct XII_FOUNDATION_DLL xiiTelemetrySubscriber
{
  xiiNetworkChannel m_Channel;
  xiiUInt64         m_uiGeneration = 0;
};

/// Application-owned diagnostics. Registration, collection, Update, and lifecycle run on the owning
/// application thread. Publish may run on producer threads and never performs network operations.
/// Stop producers and unregister their callbacks before destroying the service or unloading a plugin.
class XII_FOUNDATION_DLL xiiTelemetryService final
{
public:
  using Collect = xiiDelegate<void(xiiTelemetryService&, xiiUInt64)>;
  /// Called on the owning thread when the first consumer arrives or the last consumer leaves.
  using Demand    = xiiDelegate<void(bool)>;
  using Authorize = xiiDelegate<xiiUInt8(const xiiNetworkSessionInfo&)>;
  using Snapshot  = xiiDelegate<void(const xiiTelemetrySample&)>;

  xiiTelemetryService();
  ~xiiTelemetryService();
  xiiTelemetryService(const xiiTelemetryService&)            = delete;
  xiiTelemetryService& operator=(const xiiTelemetryService&) = delete;

  xiiNetworkError Start(const xiiTelemetryConfiguration& configuration, Authorize authorize = {});
  /// Starts same-user diagnostics with a unique native endpoint unless a name is supplied.
  xiiNetworkError StartLocal(xiiStringView sApplication, xiiStringView sEndpointName = {});
  xiiStringView   GetLocalEndpointName() const;
  xiiNetworkError Listen(const xiiNetworkLocalEndpoint& endpoint);
  xiiNetworkError Listen(const xiiNetworkEndpoint& endpoint);
  void            Shutdown();
  void            Update();

  xiiResult RegisterSource(const xiiTelemetrySourceInfo& info, Collect collect = {}, Demand demand = {});
  void      UnregisterSource(xiiUInt64 uiSource);
  bool      IsCollecting(xiiUInt64 uiSource) const;
  /// Admission transfers ownership on success. Rejection preserves the caller's sample.
  xiiResult Publish(xiiTelemetrySample&& sample, xiiNetworkChannel recipient = {});
  /// Caller-thread snapshot for encoders with independent per-consumer dictionaries.
  void GetSubscribers(xiiUInt64 uiSource, xiiDynamicArray<xiiTelemetrySubscriber>& out_subscribers) const;
  /// Explicit offline collection. No network initialization or subscribers are required.
  xiiResult                Capture(xiiUInt64 uiSource, Snapshot snapshot);
  xiiTelemetryStatistics   GetStatistics() const;
  xiiNetworkHostStatistics GetNetworkStatistics() const;
  xiiUuid                  GetInstanceId() const;

private:
  xiiTelemetryServiceState* m_pState;
};

class xiiTelemetryClientState;

/// One independently owned Inspector connection. Update dispatches handlers on its calling thread.
/// No Qt dependency or worker callback is introduced into Foundation.
class XII_FOUNDATION_DLL xiiTelemetryClient final
{
public:
  using SampleHandler       = xiiDelegate<void(const xiiTelemetrySample&)>;
  using SubscriptionHandler = xiiDelegate<void(const xiiTelemetrySubscriptionResult&)>;

  xiiTelemetryClient();
  ~xiiTelemetryClient();
  xiiTelemetryClient(const xiiTelemetryClient&)            = delete;
  xiiTelemetryClient& operator=(const xiiTelemetryClient&) = delete;

  xiiNetworkError          Connect(const xiiTelemetryConfiguration& configuration, const xiiNetworkLocalEndpoint& endpoint);
  xiiNetworkError          Connect(const xiiTelemetryConfiguration& configuration, const xiiNetworkEndpoint& endpoint, const xiiNetworkConnectOptions& options);
  void                     Shutdown();
  void                     Update();
  bool                     IsReady() const;
  const xiiTelemetryHello& GetServerInfo() const;
  xiiNetworkError          GetConnectionError() const;
  xiiNetworkDataError      Subscribe(const xiiTelemetrySubscription& subscription);
  void                     SetSampleHandler(SampleHandler handler);
  void                     SetSubscriptionHandler(SubscriptionHandler handler);

private:
  xiiNetworkError          PrepareConnection(const xiiTelemetryConfiguration& configuration);
  xiiTelemetryClientState* m_pState;
};
