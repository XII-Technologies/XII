/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

using xiiTypeVersion = xiiUInt16;

template <xiiUInt16 Size, typename AllocatorWrapper>
struct xiiHybridString;

using xiiString = xiiHybridString<32, xiiDefaultAllocatorWrapper>;

/// Interface for binary in (read) streams.
class XII_FOUNDATION_DLL xiiStreamReader
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiStreamReader);

public:
  /// Constructor.
  xiiStreamReader();

  /// Virtual destructor to ensure correct cleanup.
  virtual ~xiiStreamReader();

  /// Reads a raw number of bytes into the read buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) = 0; // [tested]

  /// Helper method to read a word value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult ReadWordValue(T* pWordValue); // [tested]

  /// Helper method to read a dword value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult ReadDWordValue(T* pDWordValue); // [tested]

  /// Helper method to read a qword value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult ReadQWordValue(T* pQWordValue); // [tested]

  /// Reads an array of elements from the stream.
  template <typename ArrayType, typename ValueType>
  xiiResult ReadArray(xiiArrayBase<ValueType, ArrayType>& ref_array); // [tested]

  /// Reads a small array of elements from the stream.
  template <typename ValueType, xiiUInt16 uiSize, typename AllocatorWrapper>
  xiiResult ReadArray(xiiSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array);

  /// Writes a C style fixed array.
  template <typename ValueType, xiiUInt32 uiSize>
  xiiResult ReadArray(ValueType (&array)[uiSize]);

  /// Reads a set.
  template <typename KeyType, typename Comparer>
  xiiResult ReadSet(xiiSetBase<KeyType, Comparer>& ref_set); // [tested]

  /// Reads a map.
  template <typename KeyType, typename ValueType, typename Comparer>
  xiiResult ReadMap(xiiMapBase<KeyType, ValueType, Comparer>& ref_map); // [tested]

  /// Read a hash table (note that the entry order is not stable).
  template <typename KeyType, typename ValueType, typename Hasher>
  xiiResult ReadHashTable(xiiHashTableBase<KeyType, ValueType, Hasher>& ref_hashTable); // [tested]

  /// Reads a string into a xiiStringBuilder.
  xiiResult ReadString(xiiStringBuilder& ref_sBuilder); // [tested]

  /// Reads a string into a xiiString.
  xiiResult ReadString(xiiString& ref_sString);


  /// Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example).
  virtual xiiUInt64 SkipBytes(xiiUInt64 uiBytesToSkip)
  {
    xiiUInt8 uiTempBuffer[1024];

    xiiUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytesToSkip)
    {
      xiiUInt64 uiBytesToRead = xiiMath::Min<xiiUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      xiiUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }

  XII_ALWAYS_INLINE xiiTypeVersion ReadVersion(xiiTypeVersion expectedMaxVersion);
};

/// Interface for binary out (write) streams.
class XII_FOUNDATION_DLL xiiStreamWriter
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiStreamWriter);

public:
  /// Constructor
  xiiStreamWriter();

  /// Virtual destructor to ensure correct cleanup
  virtual ~xiiStreamWriter();

  /// Writes a raw number of bytes from the buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) = 0; // [tested]

  /// Flushes the stream, may be implemented (not necessary to implement the interface correctly) so that user code can ensure that
  /// content is written
  virtual xiiResult Flush() // [tested]
  {
    return XII_SUCCESS;
  }

  /// Helper method to write a word value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult WriteWordValue(const T* pWordValue); // [tested]

  /// Helper method to write a dword value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult WriteDWordValue(const T* pDWordValue); // [tested]

  /// Helper method to write a qword value correctly (copes with potentially different endianess).
  template <typename T>
  xiiResult WriteQWordValue(const T* pQWordValue); // [tested]

  /// Writes a type version to the stream.
  XII_ALWAYS_INLINE void WriteVersion(xiiTypeVersion version);

  /// Writes an array of elements to the stream.
  template <typename ArrayType, typename ValueType>
  xiiResult WriteArray(const xiiArrayBase<ValueType, ArrayType>& array); // [tested]

  /// Writes a small array of elements to the stream.
  template <typename ValueType, xiiUInt16 uiSize>
  xiiResult WriteArray(const xiiSmallArrayBase<ValueType, uiSize>& array);

  /// Writes a C style fixed array
  template <typename ValueType, xiiUInt32 uiSize>
  xiiResult WriteArray(const ValueType (&array)[uiSize]);

  /// Writes a set
  template <typename KeyType, typename Comparer>
  xiiResult WriteSet(const xiiSetBase<KeyType, Comparer>& set); // [tested]

  /// Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  xiiResult WriteMap(const xiiMapBase<KeyType, ValueType, Comparer>& map); // [tested]

  /// Writes a hash table (note that the entry order might change on read)
  template <typename KeyType, typename ValueType, typename Hasher>
  xiiResult WriteHashTable(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable); // [tested]

  /// Writes a string
  xiiResult WriteString(const xiiStringView sStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>
