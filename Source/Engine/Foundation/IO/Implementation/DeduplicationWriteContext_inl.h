
#include <Foundation/IO/Stream.h>

namespace xiiInternal
{
  // This internal helper is needed to differentiate between reference and pointer which is not possible with regular function overloading
  // in this case.
  template <typename T>
  struct WriteObjectHelper
  {
    static const T* GetAddress(const T& obj) { return &obj; }
  };

  template <typename T>
  struct WriteObjectHelper<T*>
  {
    static const T* GetAddress(const T* obj) { return obj; }
  };
} // namespace xiiInternal

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& stream, const T& obj)
{
  return WriteObjectInternal(stream, xiiInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& stream, const xiiSharedPtr<T>& pObject)
{
  return WriteObjectInternal(stream, pObject.Borrow());
}

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& stream, const xiiUniquePtr<T>& pObject)
{
  return WriteObjectInternal(stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
xiiResult xiiDeduplicationWriteContext::WriteArray(xiiStreamWriter& stream, const xiiArrayBase<ValueType, ArrayType>& Array)
{
  const xiiUInt64 uiCount = Array.GetCount();
  XII_SUCCEED_OR_RETURN(stream.WriteQWordValue(&uiCount));

  for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
  {
    XII_SUCCEED_OR_RETURN(WriteObject(stream, Array[i]));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename Comparer>
xiiResult xiiDeduplicationWriteContext::WriteSet(xiiStreamWriter& stream, const xiiSetBase<KeyType, Comparer>& Set)
{
  const xiiUInt64 uiWriteSize = Set.GetCount();
  XII_SUCCEED_OR_RETURN(stream.WriteQWordValue(&uiWriteSize));

  for (const auto& item : Set)
  {
    XII_SUCCEED_OR_RETURN(WriteObject(stream, item));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiDeduplicationWriteContext::WriteMap(xiiStreamWriter& stream, const xiiMapBase<KeyType, ValueType, Comparer>& Map, WriteMapMode mode)
{
  const xiiUInt64 uiWriteSize = Map.GetCount();
  XII_SUCCEED_OR_RETURN(stream.WriteQWordValue(&uiWriteSize));

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(WriteObject(stream, It.Key()));
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(stream, It.Key()));
      XII_SUCCEED_OR_RETURN(WriteObject(stream, It.Value()));
    }
  }
  else
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(WriteObject(stream, It.Key()));
      XII_SUCCEED_OR_RETURN(WriteObject(stream, It.Value()));
    }
  }

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiDeduplicationWriteContext::WriteObjectInternal(xiiStreamWriter& stream, const T* pObject)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;

  if (pObject)
  {
    bool bIsRealObject = !m_Objects.TryGetValue(pObject, uiIndex);
    stream << bIsRealObject;

    if (bIsRealObject)
    {
      uiIndex = m_Objects.GetCount();
      m_Objects.Insert(pObject, uiIndex);

      return xiiStreamWriterUtil::Serialize<T>(stream, *pObject);
    }
    else
    {
      stream << uiIndex;
    }
  }
  else
  {
    stream << false;
    stream << xiiInvalidIndex;
  }

  return XII_SUCCESS;
}
