/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Variant.h>

/// Permissions are independent of transport authentication.
enum class xiiTelemetryPermission : xiiUInt8
{
  Read      = 1,
  Capture   = 2,
  Configure = 4,
  Console   = 8
};

namespace xiiTelemetrySources
{
  inline constexpr xiiUInt64 Counters       = 0x5849495354415432ULL;
  inline constexpr xiiUInt64 Logs           = 0x5849494C4F475332ULL;
  inline constexpr xiiUInt64 Profiling      = 0x58494950524F4632ULL;
  inline constexpr xiiUInt64 RenderGraphs   = 0x5849494752415032ULL;
  inline constexpr xiiUInt64 ProfileHistory = 0x5849494849535432ULL;
} // namespace xiiTelemetrySources

struct XII_FOUNDATION_DLL xiiTelemetrySourceInfo
{
  xiiUInt64 m_uiId = 0;
  xiiString m_sName;
  xiiString m_sCategory;
  xiiUInt32 m_uiEncodingVersion             = 1;
  xiiUInt32 m_uiMinimumIntervalMilliseconds = 33;
  xiiUInt8  m_uiPermission                  = static_cast<xiiUInt8>(xiiTelemetryPermission::Read);
  bool      m_bSnapshot                     = false;
  /// Latest-only sources coalesce pending measurements when a client's sampling interval is slower.
  bool m_bLatestOnly = false;
};

struct XII_FOUNDATION_DLL xiiTelemetryHello
{
  xiiUuid                                 m_Instance;
  xiiString                               m_sApplication;
  xiiUInt32                               m_uiProcessId   = 0;
  xiiUInt8                                m_uiPermissions = 0;
  xiiDynamicArray<xiiTelemetrySourceInfo> m_Sources;
};

struct XII_FOUNDATION_DLL xiiTelemetrySubscription
{
  xiiUInt64 m_uiRequestId            = 0;
  xiiUInt64 m_uiSource               = 0;
  xiiUInt64 m_uiGeneration           = 0;
  xiiUInt32 m_uiIntervalMilliseconds = 33;
  bool      m_bEnabled               = true;
  bool      m_bSnapshot              = false;
};

/// Source-specific scalar fields and explicitly versioned dense binary data.
/// Payload size and collection retention are bounded independently.
struct XII_FOUNDATION_DLL xiiTelemetrySample
{
  xiiUInt64            m_uiSource               = 0;
  xiiUInt64            m_uiGeneration           = 0;
  xiiUInt64            m_uiSequence             = 0;
  xiiUInt64            m_uiDropped              = 0;
  xiiUInt32            m_uiKind                 = 0;
  xiiUInt32            m_uiEncodingVersion      = 1;
  double               m_fTimestampMicroseconds = 0;
  xiiVariantDictionary m_Fields;
  xiiDataBuffer        m_Data;
};

struct XII_FOUNDATION_DLL xiiTelemetrySubscriptionResult
{
  xiiUInt64 m_uiRequestId            = 0;
  xiiUInt64 m_uiSource               = 0;
  xiiUInt64 m_uiGeneration           = 0;
  xiiUInt32 m_uiIntervalMilliseconds = 0;
  xiiUInt8  m_uiError                = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetrySourceInfo);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetryHello);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetrySubscription);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetrySample);
XII_DECLARE_REFLECTABLE_TYPE(XII_FOUNDATION_DLL, xiiTelemetrySubscriptionResult);
