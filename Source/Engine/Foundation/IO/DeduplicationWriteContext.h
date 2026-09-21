/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class xiiStreamWriter;

/// Serialization Context that de-duplicates objects when writing to a stream. Duplicated objects are identified by their address and
/// only the first occurrence is written to the stream while all subsequence occurrences are just written as an index.
class XII_FOUNDATION_DLL xiiDeduplicationWriteContext : public xiiSerializationContext<xiiDeduplicationWriteContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiDeduplicationWriteContext);

public:
  xiiDeduplicationWriteContext();
  ~xiiDeduplicationWriteContext();

  /// Writes a single object to the stream. Can be either a reference or a pointer to the object.
  template <typename T>
  xiiResult WriteObject(xiiStreamWriter& ref_stream, const T& obj); // [tested]

  /// Writes a single object to the stream.
  template <typename T>
  xiiResult WriteObject(xiiStreamWriter& ref_stream, const xiiSharedPtr<T>& pObject); // [tested]

  /// Writes a single object to the stream.
  template <typename T>
  xiiResult WriteObject(xiiStreamWriter& ref_stream, const xiiUniquePtr<T>& pObject); // [tested]

  /// Writes an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  xiiResult WriteArray(xiiStreamWriter& ref_stream, const xiiArrayBase<ValueType, ArrayType>& array); // [tested]

  /// Writes a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  xiiResult WriteSet(xiiStreamWriter& ref_stream, const xiiSetBase<KeyType, Comparer>& set); // [tested]

  enum class WriteMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// Writes a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  xiiResult WriteMap(xiiStreamWriter& ref_stream, const xiiMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode); // [tested]

private:
  template <typename T>
  xiiResult WriteObjectInternal(xiiStreamWriter& stream, const T* pObject);

  xiiHashTable<const void*, xiiUInt32> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationWriteContext_inl.h>
