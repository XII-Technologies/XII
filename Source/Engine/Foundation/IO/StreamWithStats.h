/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Stream.h>

/// A stream reader that wraps another stream to track how many bytes are read from it.
class XII_FOUNDATION_DLL xiiStreamReaderWithStats : public xiiStreamReader
{
public:
  xiiStreamReaderWithStats() = default;
  xiiStreamReaderWithStats(xiiStreamReader* pStream) :
    m_pStream(pStream)
  {
  }

  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) override
  {
    const xiiUInt64 uiRead = m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
    m_uiBytesRead += uiRead;
    return uiRead;
  }

  xiiUInt64 SkipBytes(xiiUInt64 uiBytesToSkip) override
  {
    const xiiUInt64 uiSkipped = m_pStream->SkipBytes(uiBytesToSkip);
    m_uiBytesSkipped += uiSkipped;
    return uiSkipped;
  }

  /// the stream to forward all requests to
  xiiStreamReader* m_pStream = nullptr;

  /// the number of bytes that were read from the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  xiiUInt64 m_uiBytesRead = 0;

  /// the number of bytes that were skipped from the wrapped stream
  xiiUInt64 m_uiBytesSkipped = 0;
};

/// A stream writer that wraps another stream to track how many bytes are written to it.
class XII_FOUNDATION_DLL xiiStreamWriterWithStats : public xiiStreamWriter
{
public:
  xiiStreamWriterWithStats() = default;
  xiiStreamWriterWithStats(xiiStreamWriter* pStream) :
    m_pStream(pStream)
  {
  }

  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override
  {
    m_uiBytesWritten += uiBytesToWrite;
    return m_pStream->WriteBytes(pWriteBuffer, uiBytesToWrite);
  }

  xiiResult Flush() override
  {
    return m_pStream->Flush();
  }

  /// the stream to forward all requests to
  xiiStreamWriter* m_pStream = nullptr;

  /// the number of bytes that were written to the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  xiiUInt64 m_uiBytesWritten = 0;
};
