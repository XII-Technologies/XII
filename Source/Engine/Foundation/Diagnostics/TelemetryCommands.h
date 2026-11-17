/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Serialization/NetworkDataError.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

struct XII_FOUNDATION_DLL xiiTelemetryCommandResponse
{
  xiiUInt8  m_uiError = 0;
  xiiString m_sOutput;
};

/// The complete diagnostics RPC allowlist. Console execution is supplied by the application module.
/// All methods execute on the telemetry service's application executor after session authorization.
class XII_FOUNDATION_DLL xiiTelemetryCommands
{
public:
  static constexpr xiiUInt64 ServiceId = 0x58494944434D4432ULL;
  using Console                        = xiiDelegate<xiiTelemetryCommandResponse(xiiStringView, bool)>;

  xiiTelemetryCommandResponse SetCVar(const xiiString& sName, const xiiVariant& value);
  xiiTelemetryCommandResponse Execute(const xiiString& sCommand);
  xiiTelemetryCommandResponse Complete(const xiiString& sCommand);
  Console                     m_Console;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetryCommandResponse);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetryCommands);
