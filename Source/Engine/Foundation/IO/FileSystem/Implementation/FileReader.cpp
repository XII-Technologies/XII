#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>

xiiResult xiiFileReader::Open(xiiStringView sFile, xiiUInt32 uiCacheSize /*= 1024 * 64*/, xiiFileShareMode::Enum fileShareMode /*= xiiFileShareMode::SharedReads*/, bool bAllowFileEvents /*= true*/)
{
  XII_ASSERT_DEV(m_pDataDirReader == nullptr, "The file reader is already open. (File: '{0}')", sFile);

  uiCacheSize = xiiMath::Min<xiiUInt32>(uiCacheSize, 1024 * 1024 * 32);

  m_pDataDirReader = GetFileReader(sFile, fileShareMode, bAllowFileEvents);

  if (!m_pDataDirReader)
    return XII_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheReadPosition = 0;
  m_uiBytesCached       = 0;
  m_bEOF                = false;

  return XII_SUCCESS;
}

void xiiFileReader::Close()
{
  if (m_pDataDirReader)
    m_pDataDirReader->Close();

  m_pDataDirReader = nullptr;
  m_bEOF           = true;
}

xiiUInt64 xiiFileReader::SkipBytes(xiiUInt64 uiBytesToSkip)
{
  XII_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  xiiUInt64 uiSkipPosition = 0; // how much was skipped, yet

  // if any data is still in the cache, skip that first
  {
    const xiiUInt64 uiCachedBytesLeft       = m_uiBytesCached - m_uiCacheReadPosition;
    const xiiUInt64 uiBytesSkippedFromCache = xiiMath::Min(uiCachedBytesLeft, uiBytesToSkip);
    uiSkipPosition += uiBytesSkippedFromCache;
    m_uiCacheReadPosition += uiBytesSkippedFromCache;
    uiBytesToSkip -= uiBytesSkippedFromCache;
  }

  // skip bytes on disk
  const xiiUInt64 uiBytesMeantToSkipFromDisk = uiBytesToSkip;
  const xiiUInt64 uiBytesSkippedFromDisk     = m_pDataDirReader->Skip(uiBytesToSkip);
  uiSkipPosition += uiBytesSkippedFromDisk;
  uiBytesToSkip -= uiBytesSkippedFromDisk;

  // mark end of file if suitable
  const bool endOfCacheReached    = m_uiCacheReadPosition == m_uiBytesCached;
  const bool endOfDiskDataReached = uiBytesSkippedFromDisk < uiBytesMeantToSkipFromDisk;
  if (endOfCacheReached && endOfDiskDataReached)
  {
    m_bEOF = true;
  }

  return uiSkipPosition;
}

xiiUInt64 xiiFileReader::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  XII_ASSERT_DEV(m_pDataDirReader != nullptr, "The file has not been opened (successfully).");
  if (m_bEOF)
    return 0;

  xiiUInt64 uiBufferPosition = 0; // how much was read, yet
  xiiUInt8* pBuffer          = (xiiUInt8*)pReadBuffer;

  if (uiBytesToRead > m_Cache.GetCount())
  {
    // If any data is still in the cache, use that first
    const xiiUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;

    if (uiCachedBytesLeft > 0)
    {
      xiiMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(xiiUInt32)m_uiCacheReadPosition], (xiiUInt32)uiCachedBytesLeft);
      uiBufferPosition += uiCachedBytesLeft;
      m_uiCacheReadPosition += uiCachedBytesLeft;
      uiBytesToRead -= uiCachedBytesLeft;
    }

    // Read remaining data from disk
    xiiUInt64 uiBytesReadFromDisk = 0;
    if (uiBytesToRead > 0)
    {
      uiBytesReadFromDisk = m_pDataDirReader->Read(&pBuffer[uiBufferPosition], uiBytesToRead);
      uiBufferPosition += uiBytesReadFromDisk;
    }

    // Mark EOF if we're already there
    if (uiBytesReadFromDisk == 0)
    {
      m_bEOF = true;
      return uiBufferPosition;
    }
  }
  else
  {
    while (uiBytesToRead > 0)
    {
      // Determine the chunk size to read
      xiiUInt64 uiChunkSize = uiBytesToRead;

      const xiiUInt64 uiCachedBytesLeft = m_uiBytesCached - m_uiCacheReadPosition;
      if (uiCachedBytesLeft < uiBytesToRead)
      {
        uiChunkSize = uiCachedBytesLeft;
      }

      // Copy data into the buffer
      // uiChunkSize can never be larger than the cache size, which is limited to 32 Bit
      if (uiChunkSize > 0)
      {
        xiiMemoryUtils::Copy(&pBuffer[uiBufferPosition], &m_Cache[(xiiUInt32)m_uiCacheReadPosition], (xiiUInt32)uiChunkSize);

        // Store how much was read and how much is still left to read
        uiBufferPosition += uiChunkSize;
        m_uiCacheReadPosition += uiChunkSize;
        uiBytesToRead -= uiChunkSize;
      }

      // If the cache is depleted, refill it
      // This will even be triggered if EXACTLY the amount of available bytes was read
      if (m_uiCacheReadPosition >= m_uiBytesCached)
      {
        m_uiBytesCached       = m_pDataDirReader->Read(&m_Cache[0], m_Cache.GetCount());
        m_uiCacheReadPosition = 0;

        // If nothing else could be read from the file, return the number of bytes that have been read
        if (m_uiBytesCached == 0)
        {
          // If absolutely nothing could be read, we reached the end of the file (and we actually returned everything else,
          // so the file was really read to the end).
          m_bEOF = true;
          return uiBufferPosition;
        }
      }
    }
  }

  // Return how much was read
  return uiBufferPosition;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileReader);
