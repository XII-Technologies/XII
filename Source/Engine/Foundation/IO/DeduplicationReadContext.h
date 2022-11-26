
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class xiiStreamReader;

/// \brief Serialization Context that reads de-duplicated objects from a stream and restores the pointers.
class XII_FOUNDATION_DLL xiiDeduplicationReadContext : public xiiSerializationContext<xiiDeduplicationReadContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiDeduplicationReadContext);

public:
  xiiDeduplicationReadContext();
  ~xiiDeduplicationReadContext();

  /// \brief Reads a single object inplace.
  template <typename T>
  xiiResult ReadObjectInplace(xiiStreamReader& stream, T& obj); // [tested]

  /// \brief Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& stream, T*& pObject,
                       xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& stream, xiiSharedPtr<T>& pObject,
                       xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& stream, xiiUniquePtr<T>& pObject,
                       xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  xiiResult ReadArray(xiiStreamReader& stream, xiiArrayBase<ValueType, ArrayType>& Array,
                      xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  xiiResult ReadSet(xiiStreamReader& stream, xiiSetBase<KeyType, Comparer>& Set,
                    xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  xiiResult ReadMap(xiiStreamReader& stream, xiiMapBase<KeyType, ValueType, Comparer>& Map, ReadMapMode mode, xiiAllocatorBase* pKeyAllocator = xiiFoundation::GetDefaultAllocator(),
                    xiiAllocatorBase* pValueAllocator = xiiFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  xiiResult ReadObject(xiiStreamReader& stream, T& obj, xiiAllocatorBase* pAllocator); // [tested]

  xiiDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>
