#pragma once

#if XII_ENABLED(XII_PLATFORM_BIG_ENDIAN)

template <typename T>
xiiResult xiiStreamReader::ReadWordValue(T* pWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt16));

  xiiUInt16 uiTemp;

  const xiiUInt32 uiRead = ReadBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<xiiUInt16*>(pWordValue) = xiiEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? XII_SUCCESS : XII_FAILURE;
}

template <typename T>
xiiResult xiiStreamReader::ReadDWordValue(T* pDWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt32));

  xiiUInt32 uiTemp;

  const xiiUInt32 uiRead = ReadBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<xiiUInt32*>(pDWordValue) = xiiEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? XII_SUCCESS : XII_FAILURE;
}

template <typename T>
xiiResult xiiStreamReader::ReadQWordValue(T* pQWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt64));

  xiiUInt64 uiTemp;

  const xiiUInt32 uiRead = ReadBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<xiiUInt64*>(pQWordValue) = xiiEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? XII_SUCCESS : XII_FAILURE;
}



template <typename T>
xiiResult xiiStreamWriter::WriteWordValue(const T* pWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt16));

  xiiUInt16 uiTemp = *reinterpret_cast<const xiiUInt16*>(pWordValue);
  uiTemp           = xiiEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
xiiResult xiiStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt32));

  xiiUInt32 uiTemp = *reinterpret_cast<const xiiUInt16*>(pDWordValue);
  uiTemp           = xiiEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
xiiResult xiiStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt64));

  xiiUInt64 uiTemp = *reinterpret_cast<const xiiUInt64*>(pQWordValue);
  uiTemp           = xiiEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<xiiUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T>
xiiResult xiiStreamReader::ReadWordValue(T* pWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt16));

  if (ReadBytes(reinterpret_cast<xiiUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return XII_FAILURE;

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiStreamReader::ReadDWordValue(T* pDWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt32));

  if (ReadBytes(reinterpret_cast<xiiUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return XII_FAILURE;

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiStreamReader::ReadQWordValue(T* pQWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt64));

  if (ReadBytes(reinterpret_cast<xiiUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return XII_FAILURE;

  return XII_SUCCESS;
}

template <typename T>
xiiResult xiiStreamWriter::WriteWordValue(const T* pWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt16));

  return WriteBytes(reinterpret_cast<const xiiUInt8*>(pWordValue), sizeof(T));
}

template <typename T>
xiiResult xiiStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt32));

  return WriteBytes(reinterpret_cast<const xiiUInt8*>(pDWordValue), sizeof(T));
}

template <typename T>
xiiResult xiiStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  XII_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(xiiUInt64));

  return WriteBytes(reinterpret_cast<const xiiUInt8*>(pQWordValue), sizeof(T));
}

#endif

xiiTypeVersion xiiStreamReader::ReadVersion(xiiTypeVersion expectedMaxVersion)
{
  xiiTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();

  XII_ASSERT_ALWAYS(v <= expectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, expectedMaxVersion);
  XII_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void xiiStreamWriter::WriteVersion(xiiTypeVersion version)
{
  XII_ASSERT_ALWAYS(version > 0, "Version cannot be zero.");

  WriteWordValue(&version).IgnoreResult();
}


namespace xiiStreamWriterUtil
{
  // single element serialization

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& ref_stream, const T& obj, int) -> decltype(ref_stream << obj, xiiResult(XII_SUCCESS))
  {
    ref_stream << obj;

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& ref_stream, const T& obj, long) -> decltype(obj.Serialize(ref_stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(obj.Serialize(ref_stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& ref_stream, const T& obj, float) -> decltype(obj.serialize(ref_stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.serialize(ref_stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto Serialize(xiiStreamWriter& ref_stream, const T& obj) -> decltype(SerializeImpl(ref_stream, obj, 0).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return SerializeImpl(ref_stream, obj, 0);
  }

  // serialization of array

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  template <class T>
  XII_ALWAYS_INLINE auto SerializeArrayImpl(xiiStreamWriter& ref_stream, const T* pArray, xiiUInt64 uiCount, int) -> decltype(SerializeArray(ref_stream, pArray, uiCount), xiiResult(XII_SUCCESS))
  {
    return SerializeArray(ref_stream, pArray, uiCount);
  }
#endif

  template <class T>
  xiiResult SerializeArrayImpl(xiiStreamWriter& ref_stream, const T* pArray, xiiUInt64 uiCount, long)
  {
    for (xiiUInt64 i = 0; i < uiCount; ++i)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<T>(ref_stream, pArray[i]));
    }

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE xiiResult SerializeArray(xiiStreamWriter& ref_stream, const T* pArray, xiiUInt64 uiCount)
  {
    return SerializeArrayImpl(ref_stream, pArray, uiCount, 0);
  }
} // namespace xiiStreamWriterUtil

template <typename ArrayType, typename ValueType>
xiiResult xiiStreamWriter::WriteArray(const xiiArrayBase<ValueType, ArrayType>& array)
{
  const xiiUInt64 uiCount = array.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return xiiStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetArrayPtr().GetPtr(), array.GetCount());
}

template <typename ValueType, xiiUInt16 uiSize>
xiiResult xiiStreamWriter::WriteArray(const xiiSmallArrayBase<ValueType, uiSize>& array)
{
  const xiiUInt32 uiCount = array.GetCount();
  XII_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));

  return xiiStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, xiiUInt32 uiSize>
xiiResult xiiStreamWriter::WriteArray(const ValueType (&array)[uiSize])
{
  const xiiUInt64 uiWriteSize = uiSize;
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return xiiStreamWriterUtil::SerializeArray<ValueType>(*this, Array, uiSize);
}

template <typename KeyType, typename Comparer>
xiiResult xiiStreamWriter::WriteSet(const xiiSetBase<KeyType, Comparer>& set)
{
  const xiiUInt64 uiWriteSize = set.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiStreamWriter::WriteMap(const xiiMapBase<KeyType, ValueType, Comparer>& map)
{
  const xiiUInt64 uiWriteSize = map.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = map.GetIterator(); It.IsValid(); ++It)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
xiiResult xiiStreamWriter::WriteHashTable(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable)
{
  const xiiUInt64 uiWriteSize = hashTable.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = hashTable.GetIterator(); It.IsValid(); ++It)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return XII_SUCCESS;
}

namespace xiiStreamReaderUtil
{
  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& ref_stream, T& ref_obj, int) -> decltype(ref_stream >> ref_obj, xiiResult(XII_SUCCESS))
  {
    ref_stream >> ref_obj;

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& ref_stream, T& ref_obj, long) -> decltype(ref_obj.Deserialize(ref_stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(ref_obj.Deserialize(ref_stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& ref_stream, T& ref_obj, float) -> decltype(ref_obj.deserialize(ref_stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.deserialize(stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto Deserialize(xiiStreamReader& ref_stream, T& ref_obj) -> decltype(DeserializeImpl(ref_stream, ref_obj, 0).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return DeserializeImpl(ref_stream, ref_obj, 0);
  }

  // serialization of array

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  template <class T>
  XII_ALWAYS_INLINE auto DeserializeArrayImpl(xiiStreamReader& ref_stream, T* pArray, xiiUInt64 uiCount, int) -> decltype(DeserializeArray(ref_stream, pArray, uiCount), xiiResult(XII_SUCCESS))
  {
    return DeserializeArray(ref_stream, pArray, uiCount);
  }
#endif

  template <class T>
  xiiResult DeserializeArrayImpl(xiiStreamReader& ref_stream, T* pArray, xiiUInt64 uiCount, long)
  {
    for (xiiUInt64 i = 0; i < uiCount; ++i)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(ref_stream, pArray[i]));
    }

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE xiiResult DeserializeArray(xiiStreamReader& ref_stream, T* pArray, xiiUInt64 uiCount)
  {
    return DeserializeArrayImpl(ref_stream, pArray, uiCount, 0);
  }

} // namespace xiiStreamReaderUtil

template <typename ArrayType, typename ValueType>
xiiResult xiiStreamReader::ReadArray(xiiArrayBase<ValueType, ArrayType>& ref_array)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    ref_array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(ref_array).SetCount(static_cast<xiiUInt32>(uiCount));

      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::DeserializeArray<ValueType>(*this, ref_array.GetData(), uiCount));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}

template <typename ValueType, xiiUInt16 uiSize, typename AllocatorWrapper>
xiiResult xiiStreamReader::ReadArray(xiiSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array)
{
  xiiUInt32 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt16>())
  {
    ref_array.Clear();

    if (uiCount > 0)
    {
      ref_array.SetCount(static_cast<xiiUInt16>(uiCount));

      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::DeserializeArray<ValueType>(*this, ref_array.GetData(), uiCount));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Small array uses 16 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}

template <typename ValueType, xiiUInt32 uiSize>
xiiResult xiiStreamReader::ReadArray(ValueType (&array)[uiSize])
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (static_cast<xiiUInt32>(uiCount) != uiSize)
    return XII_FAILURE;

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::DeserializeArray<ValueType>(*this, Array, uiCount));

    return XII_SUCCESS;
  }

  // Containers currently use 32 bit for counts internally. Value from file is too large.
  return XII_FAILURE;
}

template <typename KeyType, typename Comparer>
xiiResult xiiStreamReader::ReadSet(xiiSetBase<KeyType, Comparer>& ref_set)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    ref_set.Clear();

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType Item;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Item));

      ref_set.Insert(std::move(Item));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiStreamReader::ReadMap(xiiMapBase<KeyType, ValueType, Comparer>& ref_map)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    ref_map.Clear();

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   Key;
      ValueType Value;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Key));
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Value));

      ref_map.Insert(std::move(Key), std::move(Value));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Hasher>
xiiResult xiiStreamReader::ReadHashTable(xiiHashTableBase<KeyType, ValueType, Hasher>& ref_hashTable)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    ref_hashTable.Clear();
    ref_hashTable.Reserve(static_cast<xiiUInt32>(uiCount));

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   Key;
      ValueType Value;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Key));
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Value));

      ref_hashTable.Insert(std::move(Key), std::move(Value));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}
