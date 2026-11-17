/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

/// Opts this property into network serialization. Zero IDs derive from the reflected property name.
/// Missing optional fields retain the destination value (or the constructor default for new messages).
/// Metadata must be immutable after publication; refresh the registry after deliberate metadata edits.
class XII_FOUNDATION_DLL xiiNetworkAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNetworkAttribute, xiiPropertyAttribute);

public:
  explicit xiiNetworkAttribute(xiiUInt64 uiFieldId = 0, bool bOptional = false) : m_uiFieldId(uiFieldId), m_bOptional(bOptional) {}
  xiiUInt64 m_uiFieldId            = 0;
  bool      m_bOptional            = false;
  xiiUInt32 m_uiMaximumElements    = 4096;
  xiiUInt32 m_uiMaximumStringBytes = 65536;
  xiiUInt64 m_uiCodecId            = 0;
  /// Zero preserves precision. Otherwise quantize supported float/vector components with 1..24 bits.
  xiiUInt8 m_uiQuantizationBits = 0;
  double   m_fMinimum           = 0;
  double   m_fMaximum           = 1;
};

/// Preserves wire identity across type renaming. Explicitly opts empty message types into the registry.
class XII_FOUNDATION_DLL xiiNetworkIdentityAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNetworkIdentityAttribute, xiiPropertyAttribute);

public:
  explicit xiiNetworkIdentityAttribute(xiiUInt64 uiTypeId = 0) : m_uiTypeId(uiTypeId) {}
  xiiUInt64 m_uiTypeId = 0;
};

/// RPC eligibility alone grants no invocation permissions. Services require explicit authorization.
class XII_FOUNDATION_DLL xiiNetworkRpcAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNetworkRpcAttribute, xiiPropertyAttribute);

public:
  explicit xiiNetworkRpcAttribute(xiiUInt64 uiMethodId = 0) : m_uiMethodId(uiMethodId) {}
  xiiUInt64 m_uiMethodId             = 0;
  xiiUInt32 m_uiMaximumArgumentBytes = 65536;
  xiiUInt32 m_uiTimeoutMilliseconds  = 5000;
  bool      m_bAllowFireAndForget    = true;
};
