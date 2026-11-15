/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Protocol/NetworkFrame.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <utility>

namespace
{
  constexpr xiiUInt32 FrameMetadataBytes = 13;

  bool ValidFrameType(xiiNetworkFrameType type)
  {
    return type >= xiiNetworkFrameType::Hello && type <= xiiNetworkFrameType::Pong;
  }
} // namespace

xiiNetworkError xiiNetworkFrame::EncodeHeader(const xiiNetworkFrameHeader& header, xiiArrayPtr<xiiUInt8> out_bytes, xiiUInt32& out_uiCount)
{
  if (!ValidFrameType(header.m_Type))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (header.m_uiPayloadBytes > static_cast<xiiUInt32>(INT_MAX) - FrameMetadataBytes ||
      (header.m_Type != xiiNetworkFrameType::Data && header.m_uiPayloadBytes > MaximumControlPayload))
  {
    return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
  }
  xiiUInt8  uiHeader[HeaderCapacity] = {};
  xiiUInt32 uiCount                  = 0;
  xiiUInt32 uiBody                   = header.m_uiPayloadBytes + FrameMetadataBytes;
  do
  {
    uiHeader[uiCount++] = static_cast<xiiUInt8>((uiBody & 0x7FU) | (uiBody >= 128 ? 0x80U : 0U));
    uiBody >>= 7;
  } while (uiBody != 0);
  uiHeader[uiCount++] = static_cast<xiiUInt8>(header.m_Type);
  for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
  {
    uiHeader[uiCount++] = static_cast<xiiUInt8>(header.m_uiChannel >> ((3 - uiIndex) * 8));
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 8; ++uiIndex)
  {
    uiHeader[uiCount++] = static_cast<xiiUInt8>(header.m_uiSequence >> ((7 - uiIndex) * 8));
  }
  if (out_bytes.GetCount() < uiCount || out_bytes.GetPtr() == nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  std::memcpy(out_bytes.GetPtr(), uiHeader, uiCount);
  out_uiCount = uiCount;
  return {};
}

xiiNetworkFrameDecoder::xiiNetworkFrameDecoder(xiiUInt32 uiMaximumPayloadBytes)
{
  const auto error = SetMaximumPayloadBytes(uiMaximumPayloadBytes);
  if (error.Failed())
  {
    Fail(error.m_Code).IgnoreResult();
  }
}

xiiNetworkError xiiNetworkFrameDecoder::SetMaximumPayloadBytes(xiiUInt32 uiMaximumPayloadBytes)
{
  if (m_State != xiiNetworkFrameDecodeState::Header || m_uiHeaderBytes != 0 || uiMaximumPayloadBytes > static_cast<xiiUInt32>(INT_MAX) - FrameMetadataBytes)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  m_uiMaximumPayloadBytes = uiMaximumPayloadBytes;
  return {};
}

xiiNetworkError xiiNetworkFrameDecoder::Fail(xiiNetworkErrorCode code)
{
  m_State = xiiNetworkFrameDecodeState::Failed;
  m_Error = xiiNetworkError(code);
  m_Frame.m_Payload.Clear();
  return m_Error;
}

xiiNetworkError xiiNetworkFrameDecoder::Consume(xiiArrayPtr<const xiiUInt8> bytes, xiiUInt32& out_uiConsumed)
{
  out_uiConsumed = 0;
  if (bytes.GetCount() != 0 && bytes.GetPtr() == nullptr)
  {
    return Fail(xiiNetworkErrorCode::InvalidArgument);
  }
  if (m_State == xiiNetworkFrameDecodeState::Failed)
  {
    return m_Error;
  }
  while (out_uiConsumed < bytes.GetCount() && m_State == xiiNetworkFrameDecodeState::Header)
  {
    const xiiUInt8 uiByte         = bytes[out_uiConsumed++];
    m_uiHeader[m_uiHeaderBytes++] = uiByte;
    if (m_uiPrefixBytes == 0)
    {
      const xiiUInt32 uiIndex = m_uiHeaderBytes - 1;
      if ((uiIndex == 4 && (uiByte & 0xF8U) != 0) || (uiIndex != 0 && (uiByte & 0x80U) == 0 && uiByte == 0))
      {
        return Fail(xiiNetworkErrorCode::ProtocolViolation);
      }
      m_uiBodyBytes |= static_cast<xiiUInt32>(uiByte & 0x7FU) << (uiIndex * 7);
      if ((uiByte & 0x80U) != 0)
      {
        continue;
      }
      m_uiPrefixBytes = m_uiHeaderBytes;
      if (m_uiBodyBytes < FrameMetadataBytes)
      {
        return Fail(xiiNetworkErrorCode::ProtocolViolation);
      }
      if (m_uiBodyBytes - FrameMetadataBytes > m_uiMaximumPayloadBytes)
      {
        return Fail(xiiNetworkErrorCode::MessageTooLarge);
      }
    }
    if (m_uiHeaderBytes == m_uiPrefixBytes + FrameMetadataBytes)
    {
      const xiiUInt8* pMetadata = m_uiHeader + m_uiPrefixBytes;
      auto&           header    = m_Frame.m_Header;
      header.m_Type             = static_cast<xiiNetworkFrameType>(pMetadata[0]);
      if (!ValidFrameType(header.m_Type))
      {
        return Fail(xiiNetworkErrorCode::ProtocolViolation);
      }
      header.m_uiPayloadBytes = m_uiBodyBytes - FrameMetadataBytes;
      if (header.m_Type != xiiNetworkFrameType::Data && header.m_uiPayloadBytes > xiiNetworkFrame::MaximumControlPayload)
      {
        return Fail(xiiNetworkErrorCode::MessageTooLarge);
      }
      for (xiiUInt32 uiIndex = 1; uiIndex < 5; ++uiIndex)
      {
        header.m_uiChannel = (header.m_uiChannel << 8) | pMetadata[uiIndex];
      }
      for (xiiUInt32 uiIndex = 5; uiIndex < FrameMetadataBytes; ++uiIndex)
      {
        header.m_uiSequence = (header.m_uiSequence << 8) | pMetadata[uiIndex];
      }
      m_State = header.m_uiPayloadBytes == 0 ? xiiNetworkFrameDecodeState::Ready : xiiNetworkFrameDecodeState::NeedBuffer;
    }
  }
  if (m_State == xiiNetworkFrameDecodeState::Payload)
  {
    const xiiUInt32 uiCount = std::min(bytes.GetCount() - out_uiConsumed, m_Frame.m_Header.m_uiPayloadBytes - m_uiPayloadBytes);
    if (uiCount != 0)
    {
      std::memcpy(m_Frame.m_Payload.GetBytes().GetPtr() + m_uiPayloadBytes, bytes.GetPtr() + out_uiConsumed, uiCount);
      m_uiPayloadBytes += uiCount;
      out_uiConsumed += uiCount;
    }
    if (m_uiPayloadBytes == m_Frame.m_Header.m_uiPayloadBytes)
    {
      m_State = xiiNetworkFrameDecodeState::Ready;
    }
  }
  return {};
}

xiiNetworkError xiiNetworkFrameDecoder::SupplyBuffer(xiiNetworkIOBuffer&& buffer)
{
  if (m_State != xiiNetworkFrameDecodeState::NeedBuffer || !buffer.IsValid() || buffer.GetCount() != m_Frame.m_Header.m_uiPayloadBytes)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  m_Frame.m_Payload = std::move(buffer);
  m_State           = xiiNetworkFrameDecodeState::Payload;
  return {};
}

xiiNetworkError xiiNetworkFrameDecoder::TakeFrame(xiiNetworkFrame& out_frame)
{
  if (m_State != xiiNetworkFrameDecodeState::Ready)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InProgress);
  }
  out_frame = std::move(m_Frame);
  Reset();
  return {};
}

void xiiNetworkFrameDecoder::Reset()
{
  m_Frame          = xiiNetworkFrame();
  m_State          = xiiNetworkFrameDecodeState::Header;
  m_Error          = {};
  m_uiHeaderBytes  = 0;
  m_uiPrefixBytes  = 0;
  m_uiBodyBytes    = 0;
  m_uiPayloadBytes = 0;
}
