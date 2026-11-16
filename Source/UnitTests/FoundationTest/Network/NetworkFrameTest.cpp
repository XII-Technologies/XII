/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Network/NetworkBufferPool.h>
#include <Foundation/Network/Protocol/NetworkFrame.h>

#include <cstring>
#include <utility>

XII_CREATE_SIMPLE_TEST_GROUP(NetworkTransport);

XII_CREATE_SIMPLE_TEST(NetworkTransport, IncrementalFrames)
{
  for (xiiUInt32 uiSize : {0U, 1U, 114U, 115U, 16384U, 65536U})
  {
    xiiNetworkBufferPool pool;
    if (!XII_TEST_BOOL(pool.Initialize().Succeeded()))
    {
      return;
    }
    xiiNetworkFrameHeader header;
    header.m_uiChannel                                  = 0x12345678U;
    header.m_uiSequence                                 = 0x123456789ABCDEF0ULL;
    header.m_uiPayloadBytes                             = uiSize;
    xiiUInt8  uiHeader[xiiNetworkFrame::HeaderCapacity] = {};
    xiiUInt32 uiHeaderBytes                             = 0;
    if (!XII_TEST_BOOL(xiiNetworkFrame::EncodeHeader(header, uiHeader, uiHeaderBytes).Succeeded()))
    {
      return;
    }
    xiiNetworkFrameDecoder decoder;
    for (xiiUInt32 uiIndex = 0; uiIndex < uiHeaderBytes; ++uiIndex)
    {
      xiiUInt32 uiConsumed = 0;
      XII_TEST_BOOL(decoder.Consume({uiHeader + uiIndex, 1}, uiConsumed).Succeeded());
      XII_TEST_INT(uiConsumed, 1);
      XII_TEST_INT(pool.GetStatistics().m_uiAllocations, 0);
    }
    if (uiSize != 0)
    {
      XII_TEST_BOOL(decoder.GetState() == xiiNetworkFrameDecodeState::NeedBuffer);
      xiiNetworkIOBuffer buffer;
      if (!XII_TEST_BOOL(pool.Acquire(uiSize, buffer).Succeeded()) || !XII_TEST_BOOL(decoder.SupplyBuffer(std::move(buffer)).Succeeded()))
      {
        return;
      }
      const xiiUInt8 uiData[193] = {};
      xiiUInt32      uiLeft      = uiSize;
      while (uiLeft != 0)
      {
        const xiiUInt32 uiCount    = xiiMath::Min(uiLeft, 193U);
        xiiUInt32       uiConsumed = 0;
        XII_TEST_BOOL(decoder.Consume({uiData, uiCount}, uiConsumed).Succeeded());
        XII_TEST_INT(uiConsumed, uiCount);
        uiLeft -= uiConsumed;
      }
    }
    XII_TEST_BOOL(decoder.GetState() == xiiNetworkFrameDecodeState::Ready);
    xiiNetworkFrame frame;
    XII_TEST_BOOL(decoder.TakeFrame(frame).Succeeded());
    XII_TEST_INT(frame.m_Header.m_uiChannel, header.m_uiChannel);
    XII_TEST_INT(frame.m_Header.m_uiSequence, header.m_uiSequence);
    XII_TEST_INT(frame.m_Header.m_uiPayloadBytes, uiSize);
    XII_TEST_INT(frame.m_Payload.GetCount(), uiSize);
    XII_TEST_BOOL(decoder.GetState() == xiiNetworkFrameDecodeState::Header);
    frame.m_Payload.Clear();
  }
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, CoalescedAndMalformedFrames)
{
  xiiUInt8              uiBytes[128] = {};
  xiiUInt32             uiCount      = 0;
  xiiNetworkFrameHeader header;
  header.m_Type = xiiNetworkFrameType::Ping;
  XII_TEST_BOOL(xiiNetworkFrame::EncodeHeader(header, uiBytes, uiCount).Succeeded());
  std::memcpy(uiBytes + uiCount, uiBytes, uiCount);
  xiiNetworkFrameDecoder decoder;
  xiiUInt32              uiConsumed = 0;
  XII_TEST_BOOL(decoder.Consume({uiBytes, uiCount * 2}, uiConsumed).Succeeded());
  XII_TEST_INT(uiConsumed, uiCount);
  xiiNetworkFrame frame;
  XII_TEST_BOOL(decoder.TakeFrame(frame).Succeeded());
  XII_TEST_BOOL(decoder.Consume({uiBytes + uiCount, uiCount}, uiConsumed).Succeeded());
  XII_TEST_BOOL(decoder.TakeFrame(frame).Succeeded());
  const xiiUInt8 uiInvalid[][5] = {{0x80, 0, 0, 0, 0}, {0xFF, 0xFF, 0xFF, 0xFF, 8}, {0x80, 0x80, 0x80, 0x80, 0x80}};
  for (const auto& bytes : uiInvalid)
  {
    XII_TEST_BOOL(decoder.Consume(bytes, uiConsumed).Failed());
    XII_TEST_BOOL(decoder.GetState() == xiiNetworkFrameDecodeState::Failed);
    XII_TEST_BOOL(decoder.Consume({}, uiConsumed).Failed());
    decoder.Reset();
  }
  header.m_uiPayloadBytes = 65537;
  header.m_Type           = xiiNetworkFrameType::Data;
  XII_TEST_BOOL(xiiNetworkFrame::EncodeHeader(header, uiBytes, uiCount).Succeeded());
  XII_TEST_BOOL(decoder.SetMaximumPayloadBytes(65536).Succeeded());
  XII_TEST_BOOL(decoder.Consume({uiBytes, uiCount}, uiConsumed).m_Code == xiiNetworkErrorCode::MessageTooLarge);
  decoder.Reset();
  header.m_uiPayloadBytes = 0;
  XII_TEST_BOOL(xiiNetworkFrame::EncodeHeader(header, uiBytes, uiCount).Succeeded());
  uiBytes[1] = 255;
  XII_TEST_BOOL(decoder.Consume({uiBytes, uiCount}, uiConsumed).m_Code == xiiNetworkErrorCode::ProtocolViolation);
}

XII_CREATE_SIMPLE_TEST(NetworkTransport, FrameParserFuzz)
{
  // Deterministic grammar fuzzing: every prefix/split is bounded and no payload allocation occurs.
  xiiUInt32              uiRandom = 0x31415926U;
  xiiNetworkFrameDecoder decoder(4096);
  for (xiiUInt32 uiIteration = 0; uiIteration < 20000; ++uiIteration)
  {
    xiiUInt8 uiData[32] = {};
    for (auto& uiByte : uiData)
    {
      uiRandom = uiRandom * 1664525U + 1013904223U;
      uiByte   = static_cast<xiiUInt8>(uiRandom >> 24);
    }
    const xiiUInt32 uiSplit    = (uiRandom % 31) + 1;
    xiiUInt32       uiConsumed = 0;
    const auto      error      = decoder.Consume({uiData, uiSplit}, uiConsumed);
    XII_TEST_BOOL(uiConsumed <= uiSplit);
    if (error.Succeeded() && decoder.GetState() == xiiNetworkFrameDecodeState::Header)
    {
      decoder.Consume({uiData + uiSplit, 32 - uiSplit}, uiConsumed).IgnoreResult();
      XII_TEST_BOOL(uiConsumed <= 32 - uiSplit);
    }
    decoder.Reset();
  }
}
