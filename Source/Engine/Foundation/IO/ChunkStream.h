/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Stream.h>

/// A stream writer that separates data into 'chunks', which act like sub-streams.
///
/// This stream writer allows to subdivide a stream into chunks, where each chunk stores a chunk name,
/// version and size in bytes.
class XII_FOUNDATION_DLL xiiChunkStreamWriter : public xiiStreamWriter
{
public:
  /// Pass the underlying stream writer to the constructor.
  xiiChunkStreamWriter(xiiStreamWriter& ref_stream); // [tested]

  /// Writes bytes directly to the stream. Only allowed when a chunk is open (between BeginChunk / EndChunk).
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override; // [tested]

  /// Starts writing to the chunk file. Has to be the first thing that is called. The version number is written to the stream and is returned
  /// by xiiChunkStreamReader::BeginStream()
  virtual void BeginStream(xiiUInt16 uiVersion); // [tested]

  /// Stops writing to the chunk file. Has to be the last thing that is called.
  virtual void EndStream(); // [tested]

  /// Opens the next chunk for writing. Chunks cannot be nested (except by using multiple chunk format writers).
  virtual void BeginChunk(xiiStringView sName, xiiUInt32 uiVersion); // [tested]

  /// Closes the current chunk.
  virtual void EndChunk(); // [tested]


private:
  bool               m_bWritingFile;
  bool               m_bWritingChunk;
  xiiString          m_sChunkName;
  xiiDeque<xiiUInt8> m_Storage;
  xiiStreamWriter&   m_Stream;
};


/// Reader for the chunk format that xiiChunkStreamWriter writes.
///
///
class XII_FOUNDATION_DLL xiiChunkStreamReader : public xiiStreamReader
{
public:
  /// Pass the underlying stream writer to the constructor.
  xiiChunkStreamReader(xiiStreamReader& ref_stream); // [tested]

  /// Reads bytes directly from the stream. Only allowed while a valid chunk is available.
  /// Returns 0 bytes when the end of a chunk is reached, even if there are more chunks to come.
  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) override; // [tested]

  enum class EndChunkFileMode
  {
    SkipToEnd, ///< Makes sure all data is properly read, so that the stream read position is after the chunk file data. Useful if the chunk file is
               ///< embedded in another file stream.
    JustClose  ///< Just stops, leaving the stream at the last read position. This should be used if definitely nothing more needs to be read from all
               ///< underlying streams.
  };

  void SetEndChunkFileMode(EndChunkFileMode mode) { m_EndChunkFileMode = mode; } // [tested]

  /// Starts reading from the chunk file. Returns the version number that was passed to xiiChunkStreamWriter::BeginStream().
  virtual xiiUInt16 BeginStream(); // [tested]

  /// Stops reading from the chunk file. Optionally skips the remaining bytes, so that the underlying streams read position is after the chunk
  /// file content.
  virtual void EndStream(); // [tested]

  /// Describes the state of the current chunk.
  struct ChunkInfo
  {
    ChunkInfo()
    {
      m_bValid             = false;
      m_uiChunkVersion     = 0;
      m_uiChunkBytes       = 0;
      m_uiUnreadChunkBytes = 0;
    }

    bool      m_bValid;             ///< If this is false, the end of the chunk file has been reached and no further chunk is available.
    xiiString m_sChunkName;         ///< The name of the chunk.
    xiiUInt32 m_uiChunkVersion;     ///< The version number of the chunk.
    xiiUInt32 m_uiChunkBytes;       ///< The total size of the chunk.
    xiiUInt32 m_uiUnreadChunkBytes; ///< The number of bytes in the chunk that have not yet been read.
  };

  /// Returns information about the current chunk.
  const ChunkInfo& GetCurrentChunk() const { return m_ChunkInfo; } // [tested]

  /// Skips the rest of the current chunk and starts reading the next chunk.
  void NextChunk(); // [tested]

private:
  void TryReadChunkHeader();

  EndChunkFileMode m_EndChunkFileMode;
  ChunkInfo        m_ChunkInfo;

  xiiStreamReader& m_Stream;
};
