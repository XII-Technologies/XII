/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/Stream.h>

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationReadContext::ReadObjectInplace(xiiStreamReader& ref_stream, T& ref_obj)
{
  return ReadObject(ref_stream, ref_obj, nullptr);
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& stream, T& obj, xiiAllocator* pAllocator)
{
  bool bIsRealObject;
  stream >> bIsRealObject;

  XII_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(stream, obj));

  m_Objects.PushBack(&obj);

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& ref_stream, T*& ref_pObject, xiiAllocator* pAllocator)
{
  bool bIsRealObject;
  ref_stream >> bIsRealObject;

  if (bIsRealObject)
  {
    XII_ASSERT_DEBUG(pAllocator != nullptr, "Valid allocator required.");

    ref_pObject = XII_NEW(pAllocator, T);
    XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(ref_stream, *ref_pObject));

    m_Objects.PushBack(ref_pObject);
  }
  else
  {
    xiiUInt32 uiIndex;
    ref_stream >> uiIndex;

    if (uiIndex < m_Objects.GetCount())
    {
      ref_pObject = static_cast<T*>(m_Objects[uiIndex]);
    }
    else if (uiIndex == xiiInvalidIndex)
    {
      ref_pObject = nullptr;
    }
    else
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& ref_stream, xiiSharedPtr<T>& ref_pObject, xiiAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(ref_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = xiiSharedPtr<T>(ptr, pAllocator);
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& ref_stream, xiiUniquePtr<T>& ref_pObject, xiiAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(ref_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = std::move(xiiUniquePtr<T>(ptr, pAllocator));
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

template <typename ArrayType, typename ValueType>
xiiResult xiiDeduplicationReadContext::ReadArray(xiiStreamReader& ref_stream, xiiArrayBase<ValueType, ArrayType>& ref_array, xiiAllocator* pAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ref_stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(ref_array).Reserve(static_cast<xiiUInt32>(uiCount));

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, ref_array.ExpandAndGetRef(), pAllocator));
    }
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename Comparer>
xiiResult xiiDeduplicationReadContext::ReadSet(xiiStreamReader& ref_stream, xiiSetBase<KeyType, Comparer>& ref_set, xiiAllocator* pAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ref_stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_set.Clear();

  for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
  {
    KeyType key;
    XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, key, pAllocator));

    ref_set.Insert(std::move(key));
  }

  return XII_SUCCESS;
}

namespace xiiInternal
{
  // Internal helper to prevent the compiler from trying to find a de-serialization method for pointer types or other types which don't have
  // one.
  struct DeserializeHelper
  {
    template <typename T>
    static auto Deserialize(xiiStreamReader& ref_stream, T& ref_obj, xiiInt32) -> decltype(xiiStreamReaderUtil::Deserialize(ref_stream, ref_obj))
    {
      return xiiStreamReaderUtil::Deserialize(ref_stream, ref_obj);
    }

    template <typename T>
    static xiiResult Deserialize(xiiStreamReader& ref_stream, T& ref_obj, float)
    {
      XII_REPORT_FAILURE("No deserialize method available");
      return XII_FAILURE;
    }
  };
} // namespace xiiInternal

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiDeduplicationReadContext::ReadMap(xiiStreamReader& ref_stream, xiiMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode, xiiAllocator* pKeyAllocator, xiiAllocator* pValueAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ref_stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, key, pKeyAllocator));
      XII_SUCCEED_OR_RETURN(xiiInternal::DeserializeHelper::Deserialize<ValueType>(ref_stream, value, 0));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(xiiInternal::DeserializeHelper::Deserialize<KeyType>(ref_stream, key, 0));
      XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, key, pKeyAllocator));
      XII_SUCCEED_OR_RETURN(ReadObject(ref_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }

  return XII_SUCCESS;
}
