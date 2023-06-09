
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
    static const T* GetAddress(const T* pObj) { return pObj; }
  };
} // namespace xiiInternal

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& ref_stream, const T& obj)
{
  return WriteObjectInternal(ref_stream, xiiInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& ref_stream, const xiiSharedPtr<T>& pObject)
{
  return WriteObjectInternal(ref_stream, pObject.Borrow());
}

template <typename T>
XII_ALWAYS_INLINE xiiResult xiiDeduplicationWriteContext::WriteObject(xiiStreamWriter& ref_stream, const xiiUniquePtr<T>& pObject)
{
  return WriteObjectInternal(ref_stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
xiiResult xiiDeduplicationWriteContext::WriteArray(xiiStreamWriter& ref_stream, const xiiArrayBase<ValueType, ArrayType>& array)
{
  const xiiUInt64 uiCount = array.GetCount();
  XII_SUCCEED_OR_RETURN(ref_stream.WriteQWordValue(&uiCount));

  for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
  {
    XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, array[i]));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename Comparer>
xiiResult xiiDeduplicationWriteContext::WriteSet(xiiStreamWriter& ref_stream, const xiiSetBase<KeyType, Comparer>& set)
{
  const xiiUInt64 uiWriteSize = set.GetCount();
  XII_SUCCEED_OR_RETURN(ref_stream.WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, item));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiDeduplicationWriteContext::WriteMap(xiiStreamWriter& ref_stream, const xiiMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode)
{
  const xiiUInt64 uiWriteSize = map.GetCount();
  XII_SUCCEED_OR_RETURN(ref_stream.WriteQWordValue(&uiWriteSize));

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, It.Key()));
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(ref_stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(ref_stream, It.Key()));
      XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, It.Value()));
    }
  }
  else
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, It.Key()));
      XII_SUCCEED_OR_RETURN(WriteObject(ref_stream, It.Value()));
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
