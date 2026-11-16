/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Reflection/NetworkTypeRegistry.h>
#include <Foundation/Reflection/Reflection.h>

enum class xiiNetworkEncoding : xiiUInt8
{
  Tagged  = 0,
  Compact = 1
};

struct XII_FOUNDATION_DLL xiiNetworkSerializedHeader
{
  xiiNetworkEncoding m_Encoding      = xiiNetworkEncoding::Tagged;
  xiiUInt64          m_uiTypeId      = 0;
  xiiUInt32          m_uiTypeVersion = 0;
  xiiUInt64          m_uiFingerprint = 0;
};

/// Schema-based value serialization, independent of host registration and remote construction policy.
/// Source/destination objects must not be mutated concurrently. Context scratch is caller-owned.
/// Only attributed fields are applied; decoding failure leaves the destination unchanged. Reflected
/// setters must have ordinary value semantics and accept every value admitted by their field policy.
class XII_FOUNDATION_DLL xiiNetworkSerializer
{
public:
  /// Output borrows context storage until its next serialization/destruction. Failure clears output.
  /// Compact payloads require the exact recursive schema; the typed endpoint negotiates this first.
  [[nodiscard]] static xiiNetworkDataError Serialize(xiiNetworkTypeRegistry& registry, const xiiRTTI* pType, const void* pObject,
                                                     xiiNetworkSerializationContext& context, xiiArrayPtr<const xiiUInt8>& out_bytes, xiiNetworkEncoding encoding = xiiNetworkEncoding::Tagged);
  /// The caller selects an already existing, permitted object. Wire IDs never select a constructor.
  [[nodiscard]] static xiiNetworkDataError Deserialize(xiiNetworkTypeRegistry& registry, const xiiRTTI* pType, void* pObject,
                                                       xiiArrayPtr<const xiiUInt8> bytes, xiiNetworkSerializationContext& context);
  [[nodiscard]] static xiiNetworkDataError ReadHeader(xiiArrayPtr<const xiiUInt8> bytes, xiiNetworkSerializedHeader& out_header);
  template <typename T>
  [[nodiscard]] static xiiNetworkDataError Serialize(xiiNetworkTypeRegistry& registry, const T& object,
                                                     xiiNetworkSerializationContext& context, xiiArrayPtr<const xiiUInt8>& out_bytes, xiiNetworkEncoding encoding = xiiNetworkEncoding::Tagged)
  {
    return Serialize(registry, xiiGetStaticRTTI<T>(), &object, context, out_bytes, encoding);
  }
  template <typename T>
  [[nodiscard]] static xiiNetworkDataError Deserialize(xiiNetworkTypeRegistry& registry, T& object,
                                                       xiiArrayPtr<const xiiUInt8> bytes, xiiNetworkSerializationContext& context)
  {
    return Deserialize(registry, xiiGetStaticRTTI<T>(), &object, bytes, context);
  }
};
