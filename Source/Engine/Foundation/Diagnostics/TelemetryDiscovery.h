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

/// Per-user local diagnostics advertisements. A held OS file lease distinguishes live entries
/// from abandoned registrations, including process crashes and reused PIDs. Discovery grants no
/// authentication or command permissions; clients still establish an authenticated network session.
class XII_FOUNDATION_DLL xiiTelemetryDiscovery final
{
public:
  ~xiiTelemetryDiscovery();
  xiiResult        Publish(const xiiTelemetryInstance& instance);
  void             Reset();
  static xiiResult Enumerate(xiiDynamicArray<xiiTelemetryInstance>& out_instances);

private:
  xiiOSFile m_Lease;
  xiiString m_sPath;
};
