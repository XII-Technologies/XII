/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

#  include <Foundation/System/SystemInformation.h>
#  include <zstd/zstd.h>

xiiCompressedStreamReaderZstd::xiiCompressedStreamReaderZstd() = default;

xiiCompressedStreamReaderZstd::xiiCompressedStreamReaderZstd(xiiStreamReader* pInputStream)
{
  SetInputStream(pInputStream);
}

xiiCompressedStreamReaderZstd::~xiiCompressedStreamReaderZstd()
{
  if (m_pZstdDStream != nullptr)
  {
    ZSTD_freeDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
    m_pZstdDStream = nullptr;
  }
}

void xiiCompressedStreamReaderZstd::SetInputStream(xiiStreamReader* pInputStream)
{
  m_InBuffer.pos  = 0;
  m_InBuffer.size = 0;
  m_bReachedEnd   = false;
  m_pInputStream  = pInputStream;

  if (m_pZstdDStream == nullptr)
  {
    m_pZstdDStream = ZSTD_createDStream();
  }

  ZSTD_initDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
}

xiiUInt64 xiiCompressedStreamReaderZstd::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  XII_ASSERT_DEV(m_pInputStream != nullptr, "No input stream has been specified");

  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // Implement the 'skip n bytes' feature with a temp cache
  if (pReadBuffer == nullptr)
  {
    xiiUInt64 uiBytesRead = 0;
    xiiUInt8  uiTemp[1024];

    while (uiBytesToRead > 0)
    {
      const xiiUInt32 uiToRead = xiiMath::Min<xiiUInt32>(static_cast<xiiUInt32>(uiBytesToRead), 1024);

      const xiiUInt64 uiGotBytes = ReadBytes(uiTemp, uiToRead);

      uiBytesRead += uiGotBytes;
      uiBytesToRead -= uiGotBytes;

      if (uiGotBytes == 0) // prevent an endless loop
        break;
    }

    return uiBytesRead;
  }

  ZSTD_outBuffer outBuffer;
  outBuffer.dst  = pReadBuffer;
  outBuffer.pos  = 0;
  outBuffer.size = xiiMath::SafeConvertToSizeT(uiBytesToRead);

  while (outBuffer.pos < outBuffer.size)
  {
    if (RefillReadCache().Failed())
      return outBuffer.pos;

    const size_t res = ZSTD_decompressStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream), &outBuffer, reinterpret_cast<ZSTD_inBuffer*>(&m_InBuffer));
    XII_IGNORE_UNUSED(res);
    XII_ASSERT_DEV(!ZSTD_isError(res), "Decompressing the stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  if (m_InBuffer.pos == m_InBuffer.size)
  {
    // if we have reached the end, we have not yet read the zero-terminator
    // do this now, so that data that comes after the compressed stream can be read properly

    RefillReadCache().IgnoreResult();
  }

  return outBuffer.pos;
}

xiiResult xiiCompressedStreamReaderZstd::RefillReadCache()
{
  // if our input buffer is empty, we need to read more into our cache
  if (m_InBuffer.pos == m_InBuffer.size)
  {
    xiiUInt16 uiCompressedSize = 0;
    XII_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(xiiUInt16)) == sizeof(xiiUInt16), "Reading the compressed chunk size from the input stream failed.");

    m_InBuffer.pos  = 0;
    m_InBuffer.size = uiCompressedSize;

    if (uiCompressedSize > 0)
    {
      if (m_CompressedCache.GetCount() < uiCompressedSize)
      {
        m_CompressedCache.SetCountUninitialized(xiiMath::RoundUp(uiCompressedSize, 1024));

        m_InBuffer.src = m_CompressedCache.GetData();
      }

      XII_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(xiiUInt8) * uiCompressedSize) == sizeof(xiiUInt8) * uiCompressedSize, "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
    }
  }

  // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
  if (m_InBuffer.size == 0)
  {
    // in this case there is also no output that can be generated anymore
    m_bReachedEnd = true;
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiCompressedStreamWriterZstd::xiiCompressedStreamWriterZstd() = default;

xiiCompressedStreamWriterZstd::xiiCompressedStreamWriterZstd(xiiStreamWriter* pOutputStream, xiiUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, xiiUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  SetOutputStream(pOutputStream, uiMaxNumWorkerThreads, ratio, uiCompressionCacheSizeKB);
}

xiiCompressedStreamWriterZstd::~xiiCompressedStreamWriterZstd()
{
  if (m_pOutputStream != nullptr)
  {
    // NOTE: FinishCompressedStream() WILL write a couple of bytes, even if the user did not write anything.
    // If xiiCompressedStreamWriterZstd was not supposed to be used, this may end up in a corrupted output file.
    // XII_ASSERT_DEV(m_uiWrittenBytes > 0, "Output stream was set, but not a single byte was written to the compressed stream before destruction.
    // Incorrect usage?");

    FinishCompressedStream().IgnoreResult();
  }

  if (m_pZstdCStream)
  {
    ZSTD_freeCStream(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream));
    m_pZstdCStream = nullptr;
  }
}

void xiiCompressedStreamWriterZstd::SetOutputStream(xiiStreamWriter* pOutputStream, xiiUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, xiiUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  if (m_pOutputStream == pOutputStream)
    return;

  // Limit the cache to 63KB, because at 64KB we run into an endless loop due to a 16 bit overflow
  uiCompressionCacheSizeKB = xiiMath::Min(uiCompressionCacheSizeKB, 63u);

  // Finish anything done on a previous output stream
  FinishCompressedStream().IgnoreResult();

  m_uiUncompressedSize = 0;
  m_uiCompressedSize   = 0;
  m_uiWrittenBytes     = 0;

  if (pOutputStream != nullptr)
  {
    m_pOutputStream = pOutputStream;

    if (m_pZstdCStream == nullptr)
    {
      m_pZstdCStream = ZSTD_createCStream();
    }

    const xiiUInt32 uiCoreCount = (uiMaxNumWorkerThreads > 0) ? xiiMath::Clamp(xiiSystemInformation::Get().GetCPUCoreCount(), 1u, uiMaxNumWorkerThreads) : 0u;

    ZSTD_CCtx_reset(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_reset_session_only);
    ZSTD_CCtx_refCDict(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), nullptr);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_compressionLevel, (xiiInt32)ratio);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_nbWorkers, uiCoreCount);

    m_CompressedCache.SetCountUninitialized(xiiMath::Max(1U, uiCompressionCacheSizeKB) * 1024);

    m_OutBuffer.dst  = m_CompressedCache.GetData();
    m_OutBuffer.pos  = 0;
    m_OutBuffer.size = m_CompressedCache.GetCount();
  }
}

xiiResult xiiCompressedStreamWriterZstd::FinishCompressedStream()
{
  if (m_pOutputStream == nullptr)
    return XII_SUCCESS;

  if (Flush().Failed())
    return XII_FAILURE;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos  = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src  = nullptr;

  const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_end);
  XII_VERIFY(!ZSTD_isError(res), "Deinitializing the zstd compression stream failed: '{0}'", ZSTD_getErrorName(res));

  // One more flush to write out the last chunk
  if (FlushWriteCache() == XII_FAILURE)
    return XII_FAILURE;

  // Write a zero-terminator
  const xiiUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(xiiUInt16)) == XII_FAILURE)
    return XII_FAILURE;

  m_uiWrittenBytes += sizeof(xiiUInt16);
  m_pOutputStream = nullptr;

  return XII_SUCCESS;
}

xiiResult xiiCompressedStreamWriterZstd::Flush()
{
  if (m_pOutputStream == nullptr)
    return XII_SUCCESS;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos  = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src  = nullptr;

  while (ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_flush) > 0)
  {
    if (FlushWriteCache() == XII_FAILURE)
      return XII_FAILURE;
  }

  if (FlushWriteCache() == XII_FAILURE)
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiCompressedStreamWriterZstd::FlushWriteCache()
{
  if (m_pOutputStream == nullptr)
    return XII_SUCCESS;

  const xiiUInt16 uiUsedCache = static_cast<xiiUInt16>(m_OutBuffer.pos);

  if (uiUsedCache == 0)
    return XII_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(xiiUInt16)) == XII_FAILURE)
    return XII_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(xiiUInt8) * uiUsedCache) == XII_FAILURE)
    return XII_FAILURE;

  m_uiCompressedSize += uiUsedCache;
  m_uiWrittenBytes += sizeof(xiiUInt16) + uiUsedCache;

  // Reset the write position
  m_OutBuffer.pos = 0;

  return XII_SUCCESS;
}

xiiResult xiiCompressedStreamWriterZstd::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEV(m_pZstdCStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += static_cast<xiiUInt32>(uiBytesToWrite);

  ZSTD_inBuffer inBuffer;
  inBuffer.pos  = 0;
  inBuffer.src  = pWriteBuffer;
  inBuffer.size = static_cast<size_t>(uiBytesToWrite);

  while (inBuffer.pos < inBuffer.size)
  {
    if (m_OutBuffer.pos == m_OutBuffer.size)
    {
      if (FlushWriteCache() == XII_FAILURE)
        return XII_FAILURE;
    }

    const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &inBuffer, ZSTD_e_continue);

    XII_VERIFY(!ZSTD_isError(res), "Compressing the zstd stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  return XII_SUCCESS;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_CompressedStreamZstd);
