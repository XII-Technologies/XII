/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkIOBuffer.h>

/// Stable Milestone 3 wire tags. Payload serialization is an application responsibility.
enum class xiiNetworkFrameType : xiiUInt8
{
  Hello = 1,
  Welcome,
  Ready,
  Activated,
  Data,
  OpenChannel,
  ChannelAccepted,
  CloseChannel,
  Disconnect,
  Ping,
  Pong
};

struct XII_FOUNDATION_DLL xiiNetworkFrameHeader
{
  xiiNetworkFrameType m_Type           = xiiNetworkFrameType::Data;
  xiiUInt32           m_uiChannel      = 0;
  xiiUInt64           m_uiSequence     = 0;
  xiiUInt32           m_uiPayloadBytes = 0;
};

/// Move-only opaque frame; the payload lease may outlive its decoder and originating host.
struct XII_FOUNDATION_DLL xiiNetworkFrame
{
  xiiNetworkFrame()                                      = default;
  xiiNetworkFrame(const xiiNetworkFrame&)                = delete;
  xiiNetworkFrame& operator=(const xiiNetworkFrame&)     = delete;
  xiiNetworkFrame(xiiNetworkFrame&&) noexcept            = default;
  xiiNetworkFrame& operator=(xiiNetworkFrame&&) noexcept = default;
  ~xiiNetworkFrame()                                     = default;

  static constexpr xiiUInt32 HeaderCapacity        = 18;
  static constexpr xiiUInt32 MaximumControlPayload = 8192;
  xiiNetworkFrameHeader      m_Header;
  xiiNetworkIOBuffer         m_Payload;

  /// Canonical unsigned LEB128 body length, then type/u32 channel/u64 sequence in network order.
  /// Output is unchanged on failure. The body includes the 13-byte metadata and opaque payload.
  [[nodiscard]] static xiiNetworkError EncodeHeader(const xiiNetworkFrameHeader& header, xiiArrayPtr<xiiUInt8> out_bytes, xiiUInt32& out_uiCount);
};

enum class xiiNetworkFrameDecodeState : xiiUInt8
{
  Header,
  NeedBuffer,
  Payload,
  Ready,
  Failed
};

/// Allocation-free incremental framing. Not internally synchronized. Consume stops at a header
/// requiring storage or at a completed frame; loop over unconsumed bytes to decode coalesced frames.
/// Reserve peer/host receive credit after NeedBuffer, then SupplyBuffer with the validated length.
/// Zero-byte frames need no allocation. A framing failure is sticky until Reset().
class XII_FOUNDATION_DLL xiiNetworkFrameDecoder
{
public:
  explicit xiiNetworkFrameDecoder(xiiUInt32 uiMaximumPayloadBytes = 1024 * 1024);
  xiiNetworkFrameDecoder(const xiiNetworkFrameDecoder&)            = delete;
  xiiNetworkFrameDecoder& operator=(const xiiNetworkFrameDecoder&) = delete;
  xiiNetworkFrameDecoder(xiiNetworkFrameDecoder&&)                 = delete;
  xiiNetworkFrameDecoder& operator=(xiiNetworkFrameDecoder&&)      = delete;
  ~xiiNetworkFrameDecoder();

  /// Maximum may only change between frames. Values above INT_MAX - 13 are invalid.
  [[nodiscard]] xiiNetworkError SetMaximumPayloadBytes(xiiUInt32 uiMaximumPayloadBytes);
  [[nodiscard]] xiiNetworkError Consume(xiiArrayPtr<const xiiUInt8> bytes, xiiUInt32& out_uiConsumed);
  /// Requires an exact-count lease; ownership is preserved on failure.
  [[nodiscard]] xiiNetworkError SupplyBuffer(xiiNetworkIOBuffer&& buffer);
  /// Transfers a complete frame and resets the decoder. Output unchanged if not Ready.
  [[nodiscard]] xiiNetworkError TakeFrame(xiiNetworkFrame& out_frame);
  void                          Reset();
  /// True only when no partial frame prefix, header or payload remains.
  [[nodiscard]] bool                       IsAtFrameBoundary() const { return m_State == xiiNetworkFrameDecodeState::Header && m_uiHeaderBytes == 0; }
  [[nodiscard]] xiiNetworkFrameDecodeState GetState() const { return m_State; }
  /// Header fields are valid only in NeedBuffer, Payload and Ready states.
  [[nodiscard]] const xiiNetworkFrameHeader& GetHeader() const { return m_Frame.m_Header; }
  [[nodiscard]] xiiNetworkError              GetError() const { return m_Error; }

private:
  xiiNetworkError            Fail(xiiNetworkErrorCode code);
  xiiNetworkFrameDecodeState m_State = xiiNetworkFrameDecodeState::Header;
  xiiNetworkError            m_Error;
  xiiNetworkFrame            m_Frame;
  xiiUInt8                   m_uiHeader[xiiNetworkFrame::HeaderCapacity] = {};
  xiiUInt32                  m_uiHeaderBytes                             = 0;
  xiiUInt32                  m_uiPrefixBytes                             = 0;
  xiiUInt32                  m_uiBodyBytes                               = 0;
  xiiUInt32                  m_uiPayloadBytes                            = 0;
  xiiUInt32                  m_uiMaximumPayloadBytes                     = 0;
};
