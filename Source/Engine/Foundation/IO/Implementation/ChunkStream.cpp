#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/ChunkStream.h>

xiiChunkStreamWriter::xiiChunkStreamWriter(xiiStreamWriter& pStream) :
  m_Stream(pStream)
{
  m_bWritingFile  = false;
  m_bWritingChunk = false;
}

void xiiChunkStreamWriter::BeginStream(xiiUInt16 uiVersion)
{
  XII_ASSERT_DEV(!m_bWritingFile, "Already writing the file.");
  XII_ASSERT_DEV(uiVersion > 0, "The version number must be larger than 0");

  m_bWritingFile = true;

  const char* szTag = "BGNCHNK2";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
  m_Stream.WriteBytes(&uiVersion, 2).IgnoreResult();
}

void xiiChunkStreamWriter::EndStream()
{
  XII_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  XII_ASSERT_DEV(!m_bWritingChunk, "A chunk is still open for writing: '{0}'", m_sChunkName);

  m_bWritingFile = false;

  const char* szTag = "END CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
}

void xiiChunkStreamWriter::BeginChunk(const char* szName, xiiUInt32 uiVersion)
{
  XII_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  XII_ASSERT_DEV(!m_bWritingChunk, "A chunk is already open for writing: '{0}'", m_sChunkName);

  m_sChunkName = szName;

  const char* szTag = "NXT CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();

  m_Stream << m_sChunkName;
  m_Stream << uiVersion;

  m_bWritingChunk = true;
}


void xiiChunkStreamWriter::EndChunk()
{
  XII_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  XII_ASSERT_DEV(m_bWritingChunk, "No chunk is currently open.");

  m_bWritingChunk = false;

  const xiiUInt32 uiStorageSize = m_Storage.GetCount();
  m_Stream << uiStorageSize;
  /// \todo Write Chunk CRC

  for (xiiUInt32 i = 0; i < uiStorageSize;)
  {
    const xiiUInt32 uiRange = m_Storage.GetContiguousRange(i);

    XII_ASSERT_DEBUG(uiRange > 0, "Invalid contiguous range");

    m_Stream.WriteBytes(&m_Storage[i], uiRange).IgnoreResult();
    i += uiRange;
  }

  m_Storage.Clear();
}

xiiResult xiiChunkStreamWriter::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEV(m_bWritingChunk, "No chunk is currently written to");

  const xiiUInt8* pBytes = (const xiiUInt8*)pWriteBuffer;

  for (xiiUInt64 i = 0; i < uiBytesToWrite; ++i)
    m_Storage.PushBack(pBytes[i]);

  return XII_SUCCESS;
}



xiiChunkStreamReader::xiiChunkStreamReader(xiiStreamReader& stream) :
  m_Stream(stream)
{
  m_ChunkInfo.m_bValid = false;
  m_EndChunkFileMode   = EndChunkFileMode::JustClose;
}

xiiUInt64 xiiChunkStreamReader::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  XII_ASSERT_DEV(m_ChunkInfo.m_bValid, "No valid chunk available.");

  uiBytesToRead = xiiMath::Min<xiiUInt64>(uiBytesToRead, m_ChunkInfo.m_uiUnreadChunkBytes);
  m_ChunkInfo.m_uiUnreadChunkBytes -= (xiiUInt32)uiBytesToRead;

  return m_Stream.ReadBytes(pReadBuffer, uiBytesToRead);
}

xiiUInt16 xiiChunkStreamReader::BeginStream()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  xiiUInt16 uiVersion = 0;

  if (xiiStringUtils::IsEqual(szTag, "BGNCHNK2"))
  {
    m_Stream.ReadBytes(&uiVersion, 2);
  }
  else
  {
    // "BGN CHNK" is the old chunk identifier, before a version number was written
    XII_ASSERT_DEV(xiiStringUtils::IsEqual(szTag, "BGN CHNK"), "Not a valid chunk file.");
  }

  TryReadChunkHeader();
  return uiVersion;
}

void xiiChunkStreamReader::EndStream()
{
  if (m_EndChunkFileMode == EndChunkFileMode::SkipToEnd)
  {
    while (m_ChunkInfo.m_bValid)
      NextChunk();
  }
}

void xiiChunkStreamReader::TryReadChunkHeader()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  if (xiiStringUtils::IsEqual(szTag, "END CHNK"))
    return;

  if (xiiStringUtils::IsEqual(szTag, "NXT CHNK"))
  {
    m_Stream >> m_ChunkInfo.m_sChunkName;
    m_Stream >> m_ChunkInfo.m_uiChunkVersion;
    m_Stream >> m_ChunkInfo.m_uiChunkBytes;
    m_ChunkInfo.m_uiUnreadChunkBytes = m_ChunkInfo.m_uiChunkBytes;

    m_ChunkInfo.m_bValid = true;

    return;
  }

  XII_REPORT_FAILURE("Invalid chunk file, tag is '{0}'", szTag);
}

void xiiChunkStreamReader::NextChunk()
{
  if (!m_ChunkInfo.m_bValid)
    return;

  const xiiUInt64 uiToSkip  = m_ChunkInfo.m_uiUnreadChunkBytes;
  const xiiUInt64 uiSkipped = SkipBytes(uiToSkip);
  XII_VERIFY(uiSkipped == uiToSkip, "Corrupt chunk '{0}' (version {1}), tried to skip {2} bytes, could only read {3} bytes", m_ChunkInfo.m_sChunkName, m_ChunkInfo.m_uiChunkVersion, uiToSkip, uiSkipped);

  TryReadChunkHeader();
}



XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_ChunkStream);
