
#include <Foundation/IO/Stream.h>

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationReadContext::ReadObjectInplace(xiiStreamReader& stream, T& obj)
{
  return ReadObject(stream, obj, nullptr);
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& stream, T& obj, xiiAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  stream >> bIsRealObject;

  XII_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(stream, obj));

  m_Objects.PushBack(&obj);

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& stream, T*& pObject, xiiAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  stream >> bIsRealObject;

  if (bIsRealObject)
  {
    pObject = XII_NEW(pAllocator, T);
    XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(stream, *pObject));

    m_Objects.PushBack(pObject);
  }
  else
  {
    xiiUInt32 uiIndex;
    stream >> uiIndex;

    if (uiIndex < m_Objects.GetCount())
    {
      pObject = static_cast<T*>(m_Objects[uiIndex]);
    }
    else if (uiIndex == xiiInvalidIndex)
    {
      pObject = nullptr;
    }
    else
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& stream, xiiSharedPtr<T>& pObject, xiiAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(stream, ptr, pAllocator).Succeeded())
  {
    pObject = xiiSharedPtr<T>(ptr, pAllocator);
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

template <typename T>
xiiResult xiiDeduplicationReadContext::ReadObject(xiiStreamReader& stream, xiiUniquePtr<T>& pObject, xiiAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(stream, ptr, pAllocator).Succeeded())
  {
    pObject = std::move(xiiUniquePtr<T>(ptr, pAllocator));
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

template <typename ArrayType, typename ValueType>
xiiResult xiiDeduplicationReadContext::ReadArray(xiiStreamReader& stream, xiiArrayBase<ValueType, ArrayType>& Array, xiiAllocatorBase* pAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(Array).Reserve(static_cast<xiiUInt32>(uiCount));

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      XII_SUCCEED_OR_RETURN(ReadObject(stream, Array.ExpandAndGetRef(), pAllocator));
    }
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename Comparer>
xiiResult xiiDeduplicationReadContext::ReadSet(xiiStreamReader& stream, xiiSetBase<KeyType, Comparer>& Set, xiiAllocatorBase* pAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Set.Clear();

  for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
  {
    KeyType key;
    XII_SUCCEED_OR_RETURN(ReadObject(stream, key, pAllocator));

    Set.Insert(std::move(key));
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
    static auto Deserialize(xiiStreamReader& stream, T& obj, int) -> decltype(xiiStreamReaderUtil::Deserialize(stream, obj))
    {
      return xiiStreamReaderUtil::Deserialize(stream, obj);
    }

    template <typename T>
    static xiiResult Deserialize(xiiStreamReader& stream, T& obj, float)
    {
      XII_REPORT_FAILURE("No deserialize method available");
      return XII_FAILURE;
    }
  };
} // namespace xiiInternal

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiDeduplicationReadContext::ReadMap(xiiStreamReader& stream, xiiMapBase<KeyType, ValueType, Comparer>& Map, ReadMapMode mode, xiiAllocatorBase* pKeyAllocator, xiiAllocatorBase* pValueAllocator)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&uiCount));

  XII_ASSERT_DEV(uiCount < std::numeric_limits<xiiUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(ReadObject(stream, key, pKeyAllocator));
      XII_SUCCEED_OR_RETURN(xiiInternal::DeserializeHelper::Deserialize<ValueType>(stream, value, 0));

      Map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(xiiInternal::DeserializeHelper::Deserialize<KeyType>(stream, key, 0));
      XII_SUCCEED_OR_RETURN(ReadObject(stream, value, pValueAllocator));

      Map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   key;
      ValueType value;
      XII_SUCCEED_OR_RETURN(ReadObject(stream, key, pKeyAllocator));
      XII_SUCCEED_OR_RETURN(ReadObject(stream, value, pValueAllocator));

      Map.Insert(std::move(key), std::move(value));
    }
  }

  return XII_SUCCESS;
}
