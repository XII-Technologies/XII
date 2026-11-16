/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Network/Serialization/NetworkDataError.h>
#include <Foundation/Strings/StringView.h>

/// Appends to borrowed storage under a hard byte limit. Errors are sticky. Not synchronized.
class XII_FOUNDATION_DLL xiiNetworkBinaryWriter
{
public:
  xiiNetworkBinaryWriter(xiiDynamicArray<xiiUInt8>& storage, xiiUInt32 uiMaximumBytes);
  void U8(xiiUInt8 uiValue);
  void U32(xiiUInt32 uiValue);
  void U64(xiiUInt64 uiValue);
  void VarUInt(xiiUInt64 uiValue);
  void VarInt(xiiInt64 iValue);
  void Bytes(xiiArrayPtr<const xiiUInt8> bytes);
  void Text(xiiStringView sText, xiiUInt32 uiMaximumBytes);
  /// Reserves a network-order u32 length. EndBlock patches it after the bounded payload is written.
  [[nodiscard]] xiiUInt32           BeginBlock();
  void                              EndBlock(xiiUInt32 uiPosition);
  void                              Fail(xiiNetworkDataErrorCode code);
  [[nodiscard]] xiiNetworkDataError GetError() const { return m_Error; }
  [[nodiscard]] xiiUInt32           GetCount() const { return m_Storage.GetCount(); }

private:
  bool                       Reserve(xiiUInt32 uiBytes);
  xiiDynamicArray<xiiUInt8>& m_Storage;
  xiiUInt32                  m_uiMaximumBytes;
  xiiNetworkDataError        m_Error;
};

/// Bounded borrowed input. Canonical integer encodings and UTF-8 are checked in every build variant.
class XII_FOUNDATION_DLL xiiNetworkBinaryReader
{
public:
  explicit xiiNetworkBinaryReader(xiiArrayPtr<const xiiUInt8> bytes) : m_Bytes(bytes) {}
  [[nodiscard]] xiiUInt8                    U8();
  [[nodiscard]] xiiUInt32                   U32();
  [[nodiscard]] xiiUInt64                   U64();
  [[nodiscard]] xiiUInt64                   VarUInt();
  [[nodiscard]] xiiInt64                    VarInt();
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> Bytes(xiiUInt32 uiCount);
  /// Validated borrowed text; no allocation is performed.
  [[nodiscard]] xiiStringView       Text(xiiUInt32 uiMaximumBytes);
  void                              Fail(xiiNetworkDataErrorCode code);
  [[nodiscard]] xiiNetworkDataError GetError() const { return m_Error; }
  [[nodiscard]] bool                IsFinished() const { return m_Error.Succeeded() && m_uiPosition == m_Bytes.GetCount(); }
  [[nodiscard]] xiiUInt32           GetPosition() const { return m_uiPosition; }
  [[nodiscard]] xiiUInt32           GetRemaining() const { return m_Bytes.GetCount() - m_uiPosition; }

private:
  xiiArrayPtr<const xiiUInt8> m_Bytes;
  xiiUInt32                   m_uiPosition = 0;
  xiiNetworkDataError         m_Error;
};

/// FNV-1a over exact UTF-8 bytes, with the fixed 64-bit offset basis and prime. Zero is reserved.
[[nodiscard]] XII_FOUNDATION_DLL xiiUInt64 xiiNetworkStableId(xiiStringView sName);
