/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Time/Time.h>

/// Non-owning context-scoped identity. Generation checks reject recycled slots and foreign contexts.
struct XII_FOUNDATION_DLL xiiNetworkIORegistration
{
  xiiUInt64          m_uiContextId  = 0;
  xiiUInt64          m_uiGeneration = 0;
  xiiUInt32          m_uiSlot       = 0;
  xiiUInt32          m_uiWorker     = 0;
  [[nodiscard]] bool IsValid() const { return m_uiContextId != 0 && m_uiGeneration != 0; }
  bool               operator==(const xiiNetworkIORegistration&) const = default;
};

struct XII_FOUNDATION_DLL xiiNetworkIOOperation
{
  xiiUInt64          m_uiContextId  = 0;
  xiiUInt64          m_uiGeneration = 0;
  xiiUInt32          m_uiSlot       = 0;
  xiiUInt32          m_uiWorker     = 0;
  [[nodiscard]] bool IsValid() const { return m_uiContextId != 0 && m_uiGeneration != 0; }
  bool               operator==(const xiiNetworkIOOperation&) const = default;
};

enum class xiiNetworkIOOperationKind : xiiUInt8
{
  Accept,
  Connect,
  Send,
  Receive,
  SendTo,
  ReceiveFrom,
  Resolve
};

enum class xiiNetworkIOTransferMode : xiiUInt8
{
  Some,
  All
};

struct XII_FOUNDATION_DLL xiiNetworkIOOptions
{
  /// -2 selects the context default, -1 is infinite, nonnegative values are relative monotonic timeouts.
  xiiTime m_Timeout = xiiTime::MakeFromSeconds(-2);
  /// TCP transfers only. All preserves FIFO order through partial native transfers until completion.
  xiiNetworkIOTransferMode m_TransferMode = xiiNetworkIOTransferMode::All;
  xiiUInt64                m_uiUserData   = 0;
};
