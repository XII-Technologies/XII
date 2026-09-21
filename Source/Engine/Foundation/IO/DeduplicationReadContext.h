/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class xiiStreamReader;

/// Serialization Context that reads de-duplicated objects from a stream and restores the pointers.
class XII_FOUNDATION_DLL xiiDeduplicationReadContext : public xiiSerializationContext<xiiDeduplicationReadContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiDeduplicationReadContext);

public:
  xiiDeduplicationReadContext();
  ~xiiDeduplicationReadContext();

  /// Reads a single object inplace.
  template <typename T>
  xiiResult ReadObjectInplace(xiiStreamReader& ref_stream, T& ref_obj); // [tested]

  /// Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& ref_stream, T*& ref_pObject,
                       xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& ref_stream, xiiSharedPtr<T>& ref_pObject,
                       xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& ref_stream, xiiUniquePtr<T>& ref_pObject,
                       xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  xiiResult ReadArray(xiiStreamReader& ref_stream, xiiArrayBase<ValueType, ArrayType>& ref_array,
                      xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  xiiResult ReadSet(xiiStreamReader& ref_stream, xiiSetBase<KeyType, Comparer>& ref_set,
                    xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  xiiResult ReadMap(xiiStreamReader& ref_stream, xiiMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode, xiiAllocator* pKeyAllocator = xiiFoundation::GetDefaultAllocator(),
                    xiiAllocator* pValueAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& stream, T& obj, xiiAllocator* pAllocator); // [tested]

  xiiDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>
