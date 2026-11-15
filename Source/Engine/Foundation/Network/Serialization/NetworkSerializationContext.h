/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Serialization/NetworkBinaryStream.h>
#include <Foundation/Types/Variant.h>

struct XII_FOUNDATION_DLL xiiNetworkSerializationLimits
{
  xiiUInt32 m_uiMaximumPayloadBytes   = 1024 * 1024;
  xiiUInt32 m_uiMaximumStringBytes    = 65536;
  xiiUInt32 m_uiMaximumArrayElements  = 4096;
  xiiUInt32 m_uiMaximumMapEntries     = 4096;
  xiiUInt32 m_uiMaximumNestingDepth   = 16;
  xiiUInt32 m_uiMaximumObjectBytes    = 1024 * 1024;
  xiiUInt32 m_uiMaximumFields         = 1024;
  xiiUInt64 m_uiMaximumDecodedBytes   = 8 * 1024 * 1024;
  xiiUInt64 m_uiMaximumTemporaryBytes = 8 * 1024 * 1024;
};

/// Per-operation accounting, also provided to trusted custom codecs before they allocate.
class XII_FOUNDATION_DLL xiiNetworkDecodeBudget
{
public:
  explicit xiiNetworkDecodeBudget(const xiiNetworkSerializationLimits& limits) : m_Limits(limits) {}
  [[nodiscard]] xiiNetworkDataError                  Reserve(xiiUInt64 uiBytes);
  [[nodiscard]] xiiUInt64                            GetUsedBytes() const { return m_uiUsedBytes; }
  [[nodiscard]] const xiiNetworkSerializationLimits& GetLimits() const { return m_Limits; }

private:
  const xiiNetworkSerializationLimits& m_Limits;
  xiiUInt64                            m_uiUsedBytes = 0;
};

class xiiNetworkSerializationAccess;

/// Reusable scratch output. One context per concurrent operation; no shared mutable scratch buffers.
class XII_FOUNDATION_DLL xiiNetworkSerializationContext
{
public:
  xiiNetworkSerializationLimits m_Limits;
  [[nodiscard]] xiiUInt64       GetScratchMemoryUsage() const { return m_Scratch.GetHeapMemoryUsage(); }
  [[nodiscard]] xiiUInt64       GetPeakDecodedBytes() const { return m_uiPeakDecodedBytes; }

private:
  friend class xiiNetworkSerializationAccess;
  xiiDynamicArray<xiiUInt8> m_Scratch;
  xiiUInt64                 m_uiPeakDecodedBytes = 0;
};
