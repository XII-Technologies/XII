#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>

xiiResult xiiFileWriter::Open(xiiStringView sFile, xiiUInt32 uiCacheSize /*= 1024 * 1024*/, xiiFileShareMode::Enum FileShareMode /*= xiiFileShareMode::Exclusive*/, bool bAllowFileEvents /*= true*/)
{
  uiCacheSize = xiiMath::Clamp<xiiUInt32>(uiCacheSize, 1024, 1024 * 1024 * 32);

  m_pDataDirWriter = GetFileWriter(sFile, FileShareMode, bAllowFileEvents);

  if (!m_pDataDirWriter)
    return XII_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheWritePosition = 0;

  return XII_SUCCESS;
}

void xiiFileWriter::Close()
{
  if (!m_pDataDirWriter)
    return;

  Flush().IgnoreResult();

  m_pDataDirWriter->Close();
  m_pDataDirWriter = nullptr;
}

xiiResult xiiFileWriter::Flush()
{
  const xiiResult res    = m_pDataDirWriter->Write(&m_Cache[0], m_uiCacheWritePosition);
  m_uiCacheWritePosition = 0;

  return res;
}

xiiResult xiiFileWriter::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEV(m_pDataDirWriter != nullptr, "The file has not been opened (successfully).");

  if (uiBytesToWrite > m_Cache.GetCount())
  {
    // if there is more incoming data than what our cache can hold, there is no point in storing a copy
    // instead we can just pass the entire data through right away

    if (m_uiCacheWritePosition > 0)
    {
      XII_SUCCEED_OR_RETURN(Flush());
    }

    return m_pDataDirWriter->Write(pWriteBuffer, uiBytesToWrite);
  }
  else
  {
    xiiUInt8* pBuffer = (xiiUInt8*)pWriteBuffer;

    while (uiBytesToWrite > 0)
    {
      // determine chunk size to be written
      xiiUInt64 uiChunkSize = uiBytesToWrite;

      const xiiUInt64 uiRemainingCache = m_Cache.GetCount() - m_uiCacheWritePosition;

      if (uiRemainingCache < uiBytesToWrite)
        uiChunkSize = uiRemainingCache;

      // copy memory
      xiiMemoryUtils::Copy(&m_Cache[(xiiUInt32)m_uiCacheWritePosition], pBuffer, (xiiUInt32)uiChunkSize);

      pBuffer += uiChunkSize;
      m_uiCacheWritePosition += uiChunkSize;
      uiBytesToWrite -= uiChunkSize;

      // if the cache is full or nearly full, flush it to disk
      if (m_uiCacheWritePosition + 32 >= m_Cache.GetCount())
      {
        if (Flush() == XII_FAILURE)
          return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileWriter);
