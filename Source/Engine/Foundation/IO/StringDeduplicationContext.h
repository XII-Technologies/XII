/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Strings/String.h>

class xiiStreamWriter;
class xiiStreamReader;

/// This class allows for automatic deduplication of strings written to a stream.
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// xiiStreamWriter for subsequent serialization operations. Call End() once you want to finish writing
/// deduplicated strings. For a sample see StreamOperationsTest.cpp
class XII_FOUNDATION_DLL xiiStringDeduplicationWriteContext : public xiiSerializationContext<xiiStringDeduplicationWriteContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiStringDeduplicationWriteContext);

public:
  /// Setup the write context to perform string deduplication.
  xiiStringDeduplicationWriteContext(xiiStreamWriter& ref_originalStream);
  ~xiiStringDeduplicationWriteContext();

  /// Call this method to begin string deduplicaton. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  xiiStreamWriter& Begin();

  /// Ends the string deduplication and writes the string table to the original stream
  xiiResult End();

  /// Internal method to serialize a string.
  void SerializeString(const xiiStringView& sString, xiiStreamWriter& ref_writer);

  /// Returns the number of unique strings which were serialized with this instance.
  xiiUInt32 GetUniqueStringCount() const;

  /// Returns the original stream that was passed to the constructor.
  xiiStreamWriter& GetOriginalStream() { return m_OriginalStream; }

protected:
  xiiStreamWriter& m_OriginalStream;

  xiiDefaultMemoryStreamStorage m_TempStreamStorage;
  xiiMemoryStreamWriter         m_TempStreamWriter;

  xiiMap<xiiHybridString<64>, xiiUInt32> m_DeduplicatedStrings;
};

/// This class to restore strings written to a stream using a xiiStringDeduplicationWriteContext.
class XII_FOUNDATION_DLL xiiStringDeduplicationReadContext : public xiiSerializationContext<xiiStringDeduplicationReadContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiStringDeduplicationReadContext);

public:
  /// Setup the string table used internally.
  xiiStringDeduplicationReadContext(xiiStreamReader& ref_stream);
  ~xiiStringDeduplicationReadContext();

  /// Internal method to deserialize a string.
  xiiStringView DeserializeString(xiiStreamReader& ref_reader);

protected:
  xiiDynamicArray<xiiHybridString<64>> m_DeduplicatedStrings;
};
