/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

/// A stream reader that will decompress data that was stored using the xiiCompressedStreamWriterZstd.
///
/// The reader takes another reader as its source for the compressed data (e.g. a file or a memory stream).
class XII_FOUNDATION_DLL xiiCompressedStreamReaderZstd : public xiiStreamReader
{
public:
  xiiCompressedStreamReaderZstd(); // [tested]

  /// Takes an input stream as the source from which to read the compressed data.
  xiiCompressedStreamReaderZstd(xiiStreamReader* pInputStream); // [tested]

  ~xiiCompressedStreamReaderZstd(); // [tested]

  /// Configures the reader to decompress the data from the given input stream.
  ///
  /// Calling this a second time on the same instance is valid and allows to reuse the decoder, which is more efficient than creating a new
  /// one.
  void SetInputStream(xiiStreamReader* pInputStream); // [tested]

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  /// However, since this is a compressed stream, the decompression still needs to be done, so this won't save any time.
  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) override; // [tested]

private:
  xiiResult RefillReadCache();

  // local declaration to reduce #include dependencies
  struct InBufferImpl
  {
    const void* src;
    size_t      size;
    size_t      pos;
  };

  bool                           m_bReachedEnd = false;
  xiiDynamicArray<xiiUInt8>      m_CompressedCache;
  xiiStreamReader*               m_pInputStream = nullptr;
  /*ZSTD_DStream*/ void*         m_pZstdDStream = nullptr;
  /*ZSTD_inBuffer*/ InBufferImpl m_InBuffer;
};

/// A stream writer that will compress all incoming data and then passes it on into another stream.
///
/// The stream uses an internal cache of 255 Bytes to compress data, before it passes that on to the output stream.
/// It does not need to compress the entire data first, and it will not do any dynamic memory allocations.
/// Calling Flush() will write the current amount of compressed data to the output stream. Calling this frequently might reduce the
/// compression ratio and it should only be used to reduce output lag. However, there is absolutely no guarantee that all the data that was
/// put into the stream will be readable from the output stream, after calling Flush(). In fact, it is quite likely that a large amount of
/// data has still not been written to it, because it is still inside the compressor.
class XII_FOUNDATION_DLL xiiCompressedStreamWriterZstd final : public xiiStreamWriter
{
public:
  /// Specifies the compression level of the stream.
  enum class Compression
  {
    Fastest = 1,
    Fast    = 5,
    Average = 10,
    High    = 15,
    Highest = 18,     // officially up to 22, but with 20 I already encountered never-ending compression times inside zstd :(
    Default = Fastest ///< Should be preferred, good compression and good speed. Higher compression ratios save not much space but take
                      ///< considerably longer.
  };

  xiiCompressedStreamWriterZstd();

  /// The constructor takes another stream writer to pass the output into, and a compression level.
  xiiCompressedStreamWriterZstd(xiiStreamWriter* pOutputStream, xiiUInt32 uiMaxNumWorkerThreads, Compression ratio = Compression::Default, xiiUInt32 uiCompressionCacheSizeKB = 4); // [tested]

  /// Calls FinishCompressedStream() internally.
  ~xiiCompressedStreamWriterZstd(); // [tested]

  /// Configures to which other xiiStreamWriter the compressed data should be passed along.
  ///
  /// Also configures how strong the compression should be and how much data (in KB) should be cached internally before passing it
  /// to the compressor. Adjusting this cache size is only of interest, if the compressed output needs to be consumed as quickly as possible
  /// (ie sent over a network) even before the full data is compressed. Otherwise 4 KB is a good default.
  ///
  /// This has to be called before writing any bytes to the stream. It may be called by the constructor.
  /// If this is called a second time, on the same writer, the writer finishes up all work on the previous stream and can then be reused on
  /// another stream. This can prevent internal allocations, if one wants to use compression on multiple streams consecutively. It also
  /// allows to create a compressor stream early, but decide at a later pointer whether or with which stream to use it, and it will only
  /// allocate internal structures once that final decision is made.
  void SetOutputStream(xiiStreamWriter* pOutputStream, xiiUInt32 uiMaxNumWorkerThreads, Compression ratio = Compression::Default, xiiUInt32 uiCompressionCacheSizeKB = 4); // [tested]

  /// Compresses \a uiBytesToWrite from \a pWriteBuffer.
  ///
  /// Will output bursts of 256 bytes to the output stream every once in a while.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override; // [tested]

  /// Finishes the stream and writes all remaining data to the output stream.
  ///
  /// After calling this function, no more data can be written to the stream. GetCompressedSize() will return the final compressed size
  /// of the data.
  /// Note that this function is not the same as Flush(), since Flush() assumes that more data can be written to the stream afterwards,
  /// which is not the case for FinishCompressedStream().
  xiiResult FinishCompressedStream(); // [tested]

  /// Returns the size of the data in its uncompressed state.
  xiiUInt64 GetUncompressedSize() const { return m_uiUncompressedSize; } // [tested]

  /// Returns the current compressed size of the data.
  ///
  /// This value is only accurate after FinishCompressedStream() has been called. Before that it is only a rough value, because a lot of
  /// data might still be cached and not yet accounted for. Note that GetCompressedSize() returns the compressed size of the data, not the
  /// size of the data that was written to the output stream, which will be larger (1 additional byte per 255 compressed bytes, plus one
  /// zero terminator byte).
  xiiUInt64 GetCompressedSize() const { return m_uiCompressedSize; } // [tested]

  /// Returns the exact number of bytes written to the output stream so far.
  ///
  /// This includes bytes written for bookkeeping. It is strictly larger than GetCompressedSize().
  xiiUInt64 GetWrittenBytes() const { return m_uiWrittenBytes; } // [tested]

  /// Flushes the internal compressor caches and writes the compressed data to the stream.
  ///
  /// All data that was written to the compressed stream should now also be readable from the output.
  /// However, the stream is not considered 'finished' after a Flush(), since the compressor may write additional data to indicate the end.
  /// After a Flush() one may continue writing data to the compressed stream.
  ///
  /// \note Flushing the stream reduces compression effectiveness. Only in rare circumstances should it be necessary to call this manually.
  virtual xiiResult Flush() override; // [tested]

private:
  xiiResult FlushWriteCache();

  xiiUInt64 m_uiUncompressedSize = 0;
  xiiUInt64 m_uiCompressedSize   = 0;
  xiiUInt64 m_uiWrittenBytes     = 0;

  // local declaration to reduce #include dependencies
  struct OutBufferImpl
  {
    void*  dst;
    size_t size;
    size_t pos;
  };

  xiiStreamWriter*                 m_pOutputStream = nullptr;
  /*ZSTD_CStream*/ void*           m_pZstdCStream  = nullptr;
  /*ZSTD_outBuffer*/ OutBufferImpl m_OutBuffer;

  xiiDynamicArray<xiiUInt8> m_CompressedCache;
};

#endif // BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
