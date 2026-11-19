/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/Uuid.h>

struct XII_FOUNDATION_DLL xiiTelemetryInstance
{
  xiiUuid   m_Instance;
  xiiUInt32 m_uiProcessId = 0;
  xiiString m_sApplication;
  xiiString m_sEndpoint;
};

enum class xiiTelemetryDiscoveryScope : xiiUInt8
{
  Diagnostics,
  ModelContextProtocol
};

/// Per-user local diagnostics advertisements. A held OS file lease distinguishes live entries
/// from abandoned registrations, including process crashes and reused PIDs. Discovery grants no
/// authentication or command permissions; clients still establish an authenticated network session.
class XII_FOUNDATION_DLL xiiTelemetryDiscovery final
{
public:
  ~xiiTelemetryDiscovery();
  xiiResult        Publish(const xiiTelemetryInstance& instance, xiiTelemetryDiscoveryScope scope = xiiTelemetryDiscoveryScope::Diagnostics);
  void             Reset();
  static xiiResult Enumerate(xiiDynamicArray<xiiTelemetryInstance>& out_instances, xiiTelemetryDiscoveryScope scope = xiiTelemetryDiscoveryScope::Diagnostics);

private:
  xiiOSFile m_Lease;
  xiiString m_sPath;
};
