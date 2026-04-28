/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/Math/Math.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

#  include <zlib/zlib.h>

static voidpf zLibAlloc OF((voidpf opaque, uInt items, uInt size))
{
  XII_IGNORE_UNUSED(opaque);

  return XII_DEFAULT_NEW_RAW_BUFFER(xiiUInt8, xiiMath::SafeConvertToSizeT(xiiMath::SafeMultiply64(items, size)));
}

static void zLibFree OF((voidpf opaque, voidpf address))
{
  XII_IGNORE_UNUSED(opaque);

  xiiUInt8* pData = (xiiUInt8*)address;
  XII_DEFAULT_DELETE_RAW_BUFFER(pData);
}

XII_DEFINE_AS_POD_TYPE(z_stream_s);

xiiCompressedStreamReaderZip::xiiCompressedStreamReaderZip() = default;

xiiCompressedStreamReaderZip::~xiiCompressedStreamReaderZip()
{
  XII_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);
  XII_DEFAULT_DELETE(m_pZLibStream);
}

void xiiCompressedStreamReaderZip::SetInputStream(xiiStreamReader* pInputStream, xiiUInt64 uiInputSize)
{
  if (m_pZLibStream)
  {
    XII_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);
    XII_DEFAULT_DELETE(m_pZLibStream);
  }

  m_CompressedCache.SetCountUninitialized(1024 * 4);
  m_bReachedEnd          = false;
  m_pInputStream         = pInputStream;
  m_uiRemainingInputSize = uiInputSize;

  {
    m_pZLibStream = XII_DEFAULT_NEW(z_stream_s);
    xiiMemoryUtils::ZeroFill(m_pZLibStream, 1);

    m_pZLibStream->opaque = nullptr;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree  = zLibFree;

    XII_VERIFY(inflateInit2(m_pZLibStream, -MAX_WBITS) == Z_OK, "Initializing the zip stream for decompression failed: '{0}'", m_pZLibStream->msg);
  }
}

xiiUInt64 xiiCompressedStreamReaderZip::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
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


  m_pZLibStream->next_out  = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<xiiUInt32>(uiBytesToRead);
  m_pZLibStream->total_out = 0;

  while (m_pZLibStream->avail_out > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0 && m_uiRemainingInputSize > 0)
    {
      xiiUInt64 uiReadAmount = m_CompressedCache.GetCount();
      if (m_uiRemainingInputSize < uiReadAmount)
      {
        uiReadAmount = m_uiRemainingInputSize;
      }
      if (uiReadAmount == 0)
      {
        m_bReachedEnd = true;
        return m_pZLibStream->total_out;
      }

      XII_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(xiiUInt8) * uiReadAmount) == sizeof(xiiUInt8) * uiReadAmount, "Reading the compressed chunk of size {0} from the input stream failed.", uiReadAmount);
      m_pZLibStream->avail_in = static_cast<uInt>(uiReadAmount);
      m_pZLibStream->next_in  = m_CompressedCache.GetData();
      m_uiRemainingInputSize -= uiReadAmount;
    }

    const xiiInt32 iRet = inflate(m_pZLibStream, Z_SYNC_FLUSH);
    XII_ASSERT_DEV(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '{0}'", m_pZLibStream->msg);

    if (iRet == Z_STREAM_END)
    {
      m_bReachedEnd = true;
      XII_ASSERT_DEV(m_pZLibStream->avail_in == 0, "The input buffer should be depleted, but {0} bytes are still there.", m_pZLibStream->avail_in);
      return m_pZLibStream->total_out;
    }
  }

  return m_pZLibStream->total_out;
}


//////////////////////////////////////////////////////////////////////////


xiiCompressedStreamReaderZlib::xiiCompressedStreamReaderZlib(xiiStreamReader* pInputStream) :
  m_pInputStream(pInputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);
}

xiiCompressedStreamReaderZlib::~xiiCompressedStreamReaderZlib()
{
  XII_VERIFY(inflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib stream failed: '{0}'", m_pZLibStream->msg);

  XII_DEFAULT_DELETE(m_pZLibStream);
}

xiiUInt64 xiiCompressedStreamReaderZlib::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // if we have not read from the stream before, initialize everything
  if (m_pZLibStream == nullptr)
  {
    m_pZLibStream = XII_DEFAULT_NEW(z_stream_s);
    xiiMemoryUtils::ZeroFill(m_pZLibStream, 1);

    m_pZLibStream->opaque = nullptr;
    m_pZLibStream->zalloc = zLibAlloc;
    m_pZLibStream->zfree  = zLibFree;

    XII_VERIFY(inflateInit(m_pZLibStream) == Z_OK, "Initializing the zlib stream for decompression failed: '{0}'", m_pZLibStream->msg);
  }

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


  m_pZLibStream->next_out  = static_cast<Bytef*>(pReadBuffer);
  m_pZLibStream->avail_out = static_cast<xiiUInt32>(uiBytesToRead);
  m_pZLibStream->total_out = 0;

  while (m_pZLibStream->avail_out > 0)
  {
    // if our input buffer is empty, we need to read more into our cache
    if (m_pZLibStream->avail_in == 0)
    {
      xiiUInt16 uiCompressedSize = 0;
      XII_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(xiiUInt16)) == sizeof(xiiUInt16), "Reading the compressed chunk size from the input stream failed.");

      m_pZLibStream->avail_in = uiCompressedSize;
      m_pZLibStream->next_in  = m_CompressedCache.GetData();

      if (uiCompressedSize > 0)
      {
        XII_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(xiiUInt8) * uiCompressedSize) == sizeof(xiiUInt8) * uiCompressedSize, "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
      }
    }

    // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
    if (m_pZLibStream->avail_in == 0)
    {
      // in this case there is also no output that can be generated anymore
      m_bReachedEnd = true;
      return m_pZLibStream->total_out;
    }

    const xiiInt32 iRet = inflate(m_pZLibStream, Z_NO_FLUSH);
    XII_ASSERT_DEV(iRet == Z_OK || iRet == Z_STREAM_END, "Decompressing the stream failed: '{0}'", m_pZLibStream->msg);

    if (iRet == Z_STREAM_END)
    {
      m_bReachedEnd = true;

      // if we have reached the end, we have not yet read the zero-terminator
      // do this now, so that data that comes after the compressed stream can be read properly

      xiiUInt16 uiTerminator = 0;
      XII_VERIFY(m_pInputStream->ReadBytes(&uiTerminator, sizeof(xiiUInt16)) == sizeof(xiiUInt16), "Reading the compressed stream terminator failed.");

      XII_ASSERT_DEV(uiTerminator == 0, "Unexpected Stream Terminator: {0}", uiTerminator);
      XII_ASSERT_DEV(m_pZLibStream->avail_in == 0, "The input buffer should be depleted, but {0} bytes are still there.", m_pZLibStream->avail_in);
      return m_pZLibStream->total_out;
    }
  }

  return m_pZLibStream->total_out;
}


xiiCompressedStreamWriterZlib::xiiCompressedStreamWriterZlib(xiiStreamWriter* pOutputStream, Compression ratio) :
  m_pOutputStream(pOutputStream)
{
  m_CompressedCache.SetCountUninitialized(1024 * 4);

  m_pZLibStream = XII_DEFAULT_NEW(z_stream_s);

  xiiMemoryUtils::ZeroFill(m_pZLibStream, 1);

  m_pZLibStream->opaque    = nullptr;
  m_pZLibStream->zalloc    = zLibAlloc;
  m_pZLibStream->zfree     = zLibFree;
  m_pZLibStream->next_out  = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();
  m_pZLibStream->total_out = 0;

  XII_VERIFY(deflateInit(m_pZLibStream, ratio) == Z_OK, "Initializing the zlib stream for compression failed: '{0}'", m_pZLibStream->msg);
}

xiiCompressedStreamWriterZlib::~xiiCompressedStreamWriterZlib()
{
  CloseStream().IgnoreResult();
}

xiiResult xiiCompressedStreamWriterZlib::CloseStream()
{
  if (m_pZLibStream == nullptr)
    return XII_SUCCESS;

  xiiInt32 iRes = Z_OK;
  while (iRes == Z_OK)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == XII_FAILURE)
        return XII_FAILURE;
    }

    iRes = deflate(m_pZLibStream, Z_FINISH);
    XII_ASSERT_DEV(iRes == Z_STREAM_END || iRes == Z_OK, "Finishing the stream failed: '{0}'", m_pZLibStream->msg);
  }

  // one more flush to write out the last chunk
  if (Flush() == XII_FAILURE)
    return XII_FAILURE;

  // write a zero-terminator
  const xiiUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(xiiUInt16)) == XII_FAILURE)
    return XII_FAILURE;

  XII_VERIFY(deflateEnd(m_pZLibStream) == Z_OK, "Deinitializing the zlib compression stream failed: '{0}'", m_pZLibStream->msg);
  XII_DEFAULT_DELETE(m_pZLibStream);

  return XII_SUCCESS;
}

xiiResult xiiCompressedStreamWriterZlib::Flush()
{
  if (m_pZLibStream == nullptr)
    return XII_SUCCESS;

  const xiiUInt16 uiUsedCache = static_cast<xiiUInt16>(m_pZLibStream->total_out);

  if (uiUsedCache == 0)
    return XII_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(xiiUInt16)) == XII_FAILURE)
    return XII_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(xiiUInt8) * uiUsedCache) == XII_FAILURE)
    return XII_FAILURE;

  m_uiCompressedSize += uiUsedCache;

  m_pZLibStream->total_out = 0;
  m_pZLibStream->next_out  = m_CompressedCache.GetData();
  m_pZLibStream->avail_out = m_CompressedCache.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiCompressedStreamWriterZlib::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEV(m_pZLibStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += uiBytesToWrite;

  m_pZLibStream->next_in  = static_cast<Bytef*>(const_cast<void*>(pWriteBuffer)); // C libraries suck at type safety
  m_pZLibStream->avail_in = static_cast<xiiUInt32>(uiBytesToWrite);
  m_pZLibStream->total_in = 0;

  while (m_pZLibStream->avail_in > 0)
  {
    if (m_pZLibStream->avail_out == 0)
    {
      if (Flush() == XII_FAILURE)
        return XII_FAILURE;
    }

    XII_VERIFY(deflate(m_pZLibStream, Z_NO_FLUSH) == Z_OK, "Compressing the zlib stream failed: '{0}'", m_pZLibStream->msg);
  }

  return XII_SUCCESS;
}

#endif // BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
