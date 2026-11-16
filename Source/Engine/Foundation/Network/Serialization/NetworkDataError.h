/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkError.h>

enum class xiiNetworkDataErrorCode : xiiUInt8
{
  Success,
  InvalidArgument,
  UnknownType,
  UnsupportedProperty,
  InvalidAttribute,
  IdentifierCollision,
  RegistryUnavailable,
  SchemaMismatch,
  MissingRequiredField,
  InvalidFieldType,
  InvalidEncoding,
  Truncated,
  SizeLimit,
  AllocationLimit,
  UnsupportedNestedType,
  CustomCodecFailure,
  Unauthorized,
  UnknownMessage,
  UnknownRpc,
  UnsupportedSignature,
  TimedOut,
  Cancelled,
  Disconnected,
  Backpressure,
  HandlerRemoved,
  ExecutionFailed,
  TransportFailure,
  NotNegotiated,
  DuplicateRequest
};

/// Structured, allocation-free diagnostics. IDs identify metadata without exposing payloads.
struct [[nodiscard]] XII_FOUNDATION_DLL xiiNetworkDataError
{
  constexpr xiiNetworkDataError() = default;
  constexpr explicit xiiNetworkDataError(xiiNetworkDataErrorCode code, xiiUInt64 uiType = 0, xiiUInt64 uiField = 0, xiiUInt32 uiOffset = 0) :
    m_Code(code), m_uiType(uiType), m_uiField(uiField), m_uiOffset(uiOffset) {}
  [[nodiscard]] bool        Succeeded() const { return m_Code == xiiNetworkDataErrorCode::Success; }
  [[nodiscard]] bool        Failed() const { return !Succeeded(); }
  [[nodiscard]] xiiResult   GetResult() const { return Succeeded() ? XII_SUCCESS : XII_FAILURE; }
  void                      IgnoreResult() const {}
  [[nodiscard]] const char* GetCodeName() const;

  xiiNetworkDataErrorCode m_Code     = xiiNetworkDataErrorCode::Success;
  xiiUInt64               m_uiType   = 0;
  xiiUInt64               m_uiField  = 0;
  xiiUInt32               m_uiOffset = 0;
  xiiNetworkError         m_Transport;
};
