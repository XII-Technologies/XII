/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamWithStats.h>

xiiStreamReaderWithStats::xiiStreamReaderWithStats(xiiStreamReader* pStream) :
  m_pStream(pStream)
{
}

xiiStreamReaderWithStats::~xiiStreamReaderWithStats() = default;

xiiUInt64 xiiStreamReaderWithStats::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  const auto read = m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
  m_uiBytesRead += read;
  return read;
}

xiiUInt64 xiiStreamReaderWithStats::SkipBytes(xiiUInt64 uiBytesToSkip)
{
  const auto skipped = m_pStream->SkipBytes(uiBytesToSkip);
  m_uiBytesSkipped += skipped;
  return skipped;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamWithStats);
