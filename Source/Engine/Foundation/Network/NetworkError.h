/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// Stable networking errors. Success may describe a partial stream transfer; inspect the byte count.
enum class xiiNetworkErrorCode : xiiUInt8
{
  Success,
  InvalidArgument,
  InvalidAddress,
  UnsupportedAddressFamily,
  AddressInUse,
  AddressNotAvailable,
  PermissionDenied,
  ConnectionRefused,
  ConnectionReset,
  ConnectionAborted,
  ConnectionClosed,
  NetworkUnreachable,
  HostUnreachable,
  TimedOut,
  InProgress,
  WouldBlock,
  Interrupted,
  ResourceExhausted,
  MessageTooLarge,
  Truncated,
  UnsupportedOperation,
  InitializationFailed,
  NotOpen,
  AlreadyOpen,
  NotConnected,
  AlreadyConnected,
  NameNotFound,
  NameResolutionFailed,
  Unknown
};

/// Identifies the namespace of the preserved native error code.
enum class xiiNetworkErrorDomain : xiiUInt8
{
  None,
  Socket,
  Resolver,
  Initialization
};

/// Allocation-free result. Expected conditions (including WouldBlock and InProgress) are not success.
struct [[nodiscard]] XII_FOUNDATION_DLL xiiNetworkError
{
  constexpr xiiNetworkError() = default;
  constexpr xiiNetworkError(xiiNetworkErrorCode code, xiiInt32 iNativeCode = 0, xiiNetworkErrorDomain domain = xiiNetworkErrorDomain::None) :
    m_Code(code), m_Domain(domain), m_iNativeCode(iNativeCode) {}

  [[nodiscard]] constexpr bool Succeeded() const { return m_Code == xiiNetworkErrorCode::Success; }
  [[nodiscard]] constexpr bool Failed() const { return !Succeeded(); }
  [[nodiscard]] xiiResult      GetResult() const { return Succeeded() ? XII_SUCCESS : XII_FAILURE; }
  void                         IgnoreResult() const {}
  [[nodiscard]] const char*    GetCodeName() const;

  xiiNetworkErrorCode   m_Code        = xiiNetworkErrorCode::Success;
  xiiNetworkErrorDomain m_Domain      = xiiNetworkErrorDomain::None;
  xiiInt32              m_iNativeCode = 0;
};

/// One native transfer, never an implicit send-all/receive-all loop. Bytes are valid even on Truncated.
struct [[nodiscard]] XII_FOUNDATION_DLL xiiNetworkOperationResult
{
  xiiNetworkError    m_Error;
  xiiUInt32          m_uiBytesTransferred = 0;
  [[nodiscard]] bool Succeeded() const { return m_Error.Succeeded(); }
  [[nodiscard]] bool Failed() const { return m_Error.Failed(); }
};
