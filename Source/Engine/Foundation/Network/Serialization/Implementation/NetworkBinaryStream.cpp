/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Serialization/NetworkBinaryStream.h>

#include <bit>
#include <cstring>

namespace
{
  bool ValidText(xiiStringView sText)
  {
    return sText.IsEmpty() || (std::memchr(sText.GetStartPointer(), 0, sText.GetElementCount()) == nullptr && utf8::is_valid(sText.GetStartPointer(), sText.GetEndPointer()));
  }
} // namespace

xiiNetworkBinaryWriter::xiiNetworkBinaryWriter(xiiDynamicArray<xiiUInt8>& storage, xiiUInt32 uiMaximumBytes) :
  m_Storage(storage), m_uiMaximumBytes(uiMaximumBytes)
{
  if (storage.GetCount() > uiMaximumBytes)
  {
    Fail(xiiNetworkDataErrorCode::SizeLimit);
  }
}
void xiiNetworkBinaryWriter::Fail(xiiNetworkDataErrorCode code)
{
  if (m_Error.Succeeded())
  {
    m_Error = xiiNetworkDataError(code, 0, 0, GetCount());
  }
}
bool xiiNetworkBinaryWriter::Reserve(xiiUInt32 uiBytes)
{
  if (m_Error.Failed())
  {
    return false;
  }
  if (uiBytes > m_uiMaximumBytes - GetCount())
  {
    Fail(xiiNetworkDataErrorCode::SizeLimit);
    return false;
  }
  return true;
}
void xiiNetworkBinaryWriter::U8(xiiUInt8 uiValue)
{
  if (Reserve(1))
  {
    m_Storage.PushBack(uiValue);
  }
}
void xiiNetworkBinaryWriter::U32(xiiUInt32 uiValue)
{
  for (int iShift = 24; iShift >= 0; iShift -= 8)
  {
    U8(static_cast<xiiUInt8>(uiValue >> iShift));
  }
}
void xiiNetworkBinaryWriter::U64(xiiUInt64 uiValue)
{
  U32(static_cast<xiiUInt32>(uiValue >> 32));
  U32(static_cast<xiiUInt32>(uiValue));
}
void xiiNetworkBinaryWriter::VarUInt(xiiUInt64 uiValue)
{
  do
  {
    const auto uiByte = static_cast<xiiUInt8>(uiValue & 0x7FU);
    uiValue >>= 7;
    U8(static_cast<xiiUInt8>(uiByte | (uiValue != 0 ? 0x80U : 0U)));
  } while (uiValue != 0 && m_Error.Succeeded());
}
void xiiNetworkBinaryWriter::VarInt(xiiInt64 iValue) { VarUInt((std::bit_cast<xiiUInt64>(iValue) << 1) ^ (iValue < 0 ? ~xiiUInt64(0) : 0)); }
void xiiNetworkBinaryWriter::Bytes(xiiArrayPtr<const xiiUInt8> bytes)
{
  if (Reserve(bytes.GetCount()))
  {
    m_Storage.PushBackRange(bytes);
  }
}
void xiiNetworkBinaryWriter::Text(xiiStringView sText, xiiUInt32 uiMaximumBytes)
{
  if (sText.GetElementCount() > uiMaximumBytes)
  {
    Fail(xiiNetworkDataErrorCode::SizeLimit);
    return;
  }
  if (!ValidText(sText))
  {
    Fail(xiiNetworkDataErrorCode::InvalidEncoding);
    return;
  }
  VarUInt(sText.GetElementCount());
  Bytes({reinterpret_cast<const xiiUInt8*>(sText.GetStartPointer()), sText.GetElementCount()});
}
xiiUInt32 xiiNetworkBinaryWriter::BeginBlock()
{
  const auto uiPosition = GetCount();
  U32(0);
  return uiPosition;
}
void xiiNetworkBinaryWriter::EndBlock(xiiUInt32 uiPosition)
{
  if (m_Error.Failed())
  {
    return;
  }
  if (uiPosition > GetCount() || GetCount() - uiPosition < 4)
  {
    Fail(xiiNetworkDataErrorCode::InvalidArgument);
    return;
  }
  const auto uiBytes = GetCount() - uiPosition - 4;
  for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
  {
    m_Storage[uiPosition + uiIndex] = static_cast<xiiUInt8>(uiBytes >> (24 - uiIndex * 8));
  }
}
void xiiNetworkBinaryReader::Fail(xiiNetworkDataErrorCode code)
{
  if (m_Error.Succeeded())
  {
    m_Error = xiiNetworkDataError(code, 0, 0, m_uiPosition);
  }
}
xiiArrayPtr<const xiiUInt8> xiiNetworkBinaryReader::Bytes(xiiUInt32 uiCount)
{
  if (m_Error.Failed())
  {
    return {};
  }
  if (uiCount > GetRemaining())
  {
    Fail(xiiNetworkDataErrorCode::Truncated);
    return {};
  }
  const auto result = m_Bytes.GetSubArray(m_uiPosition, uiCount);
  m_uiPosition += uiCount;
  return result;
}
xiiUInt8 xiiNetworkBinaryReader::U8()
{
  const auto bytes = Bytes(1);
  return bytes.IsEmpty() ? 0 : bytes[0];
}
xiiUInt32 xiiNetworkBinaryReader::U32()
{
  xiiUInt32 uiValue = 0;
  for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
  {
    uiValue = (uiValue << 8) | U8();
  }
  return uiValue;
}
xiiUInt64 xiiNetworkBinaryReader::U64()
{
  const auto uiHigh = U32();
  return (static_cast<xiiUInt64>(uiHigh) << 32) | U32();
}
xiiUInt64 xiiNetworkBinaryReader::VarUInt()
{
  xiiUInt64 uiValue = 0;
  for (xiiUInt32 uiIndex = 0; uiIndex < 10; ++uiIndex)
  {
    const auto uiByte = U8();
    if (m_Error.Failed())
    {
      return 0;
    }
    if ((uiIndex == 9 && uiByte > 1) || (uiIndex != 0 && uiByte == 0))
    {
      Fail(xiiNetworkDataErrorCode::InvalidEncoding);
      return 0;
    }
    uiValue |= static_cast<xiiUInt64>(uiByte & 0x7FU) << (7 * uiIndex);
    if ((uiByte & 0x80U) == 0)
    {
      return uiValue;
    }
  }
  Fail(xiiNetworkDataErrorCode::InvalidEncoding);
  return 0;
}
xiiInt64 xiiNetworkBinaryReader::VarInt()
{
  const auto uiValue = VarUInt();
  return std::bit_cast<xiiInt64>((uiValue >> 1) ^ (xiiUInt64(0) - (uiValue & 1)));
}
xiiStringView xiiNetworkBinaryReader::Text(xiiUInt32 uiMaximumBytes)
{
  const auto uiCount = VarUInt();
  if (uiCount > uiMaximumBytes)
  {
    Fail(xiiNetworkDataErrorCode::SizeLimit);
    return {};
  }
  const auto          bytes = Bytes(static_cast<xiiUInt32>(uiCount));
  const xiiStringView text(reinterpret_cast<const char*>(bytes.GetPtr()), bytes.GetCount());
  if (!ValidText(text))
  {
    Fail(xiiNetworkDataErrorCode::InvalidEncoding);
    return {};
  }
  return text;
}
xiiUInt64 xiiNetworkStableId(xiiStringView sName)
{
  xiiUInt64 uiHash = 14695981039346656037ULL;
  for (const char* pByte = sName.GetStartPointer(); pByte != sName.GetEndPointer(); ++pByte)
  {
    uiHash = (uiHash ^ static_cast<xiiUInt8>(*pByte)) * 1099511628211ULL;
  }
  return uiHash;
}
