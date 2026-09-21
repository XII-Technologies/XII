/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

/// A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 32 bit hash value for any kind of data.
class XII_FOUNDATION_DLL xiiHashStreamWriter32 : public xiiStreamWriter
{
public:
  /// Pass an initial seed for the hash calculation.
  xiiHashStreamWriter32(xiiUInt32 uiSeed = 0);
  ~xiiHashStreamWriter32();

  /// Writes bytes directly to the stream.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override;

  /// Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  xiiUInt32 GetHashValue() const;

private:
  void* m_pState = nullptr;
};

//////////////////////////////////////////////////////////////////////////

/// A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 64 bit hash value for any kind of data.
class XII_FOUNDATION_DLL xiiHashStreamWriter64 : public xiiStreamWriter
{
public:
  /// Pass an initial seed for the hash calculation.
  xiiHashStreamWriter64(xiiUInt64 uiSeed = 0);
  ~xiiHashStreamWriter64();

  /// Writes bytes directly to the stream.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override;

  /// Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  xiiUInt64 GetHashValue() const;

private:
  void* m_pState = nullptr;
};
