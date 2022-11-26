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

xiiTypeVersion xiiStreamReader::ReadVersion(xiiTypeVersion uiExpectedMaxVersion)
{
  xiiTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();

  XII_ASSERT_ALWAYS(v <= uiExpectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, uiExpectedMaxVersion);
  XII_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void xiiStreamWriter::WriteVersion(xiiTypeVersion uiVersion)
{
  XII_ASSERT_ALWAYS(uiVersion > 0, "Version cannot be zero.");

  WriteWordValue(&uiVersion).IgnoreResult();
}


namespace xiiStreamWriterUtil
{
  // single element serialization

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& stream, const T& Obj, int) -> decltype(stream << Obj, xiiResult(XII_SUCCESS))
  {
    stream << Obj;

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& stream, const T& Obj, long) -> decltype(Obj.Serialize(stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.Serialize(stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto SerializeImpl(xiiStreamWriter& stream, const T& Obj, float) -> decltype(Obj.serialize(stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.serialize(stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto Serialize(xiiStreamWriter& stream, const T& Obj) -> decltype(SerializeImpl(stream, Obj, 0).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return SerializeImpl(stream, Obj, 0);
  }

  // serialization of array

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  template <class T>
  XII_ALWAYS_INLINE auto SerializeArrayImpl(xiiStreamWriter& stream, const T* pArray, xiiUInt64 uiCount, int) -> decltype(SerializeArray(stream, pArray, uiCount), xiiResult(XII_SUCCESS))
  {
    return SerializeArray(stream, pArray, uiCount);
  }
#endif

  template <class T>
  xiiResult SerializeArrayImpl(xiiStreamWriter& stream, const T* pArray, xiiUInt64 uiCount, long)
  {
    for (xiiUInt64 i = 0; i < uiCount; ++i)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<T>(stream, pArray[i]));
    }

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE xiiResult SerializeArray(xiiStreamWriter& stream, const T* pArray, xiiUInt64 uiCount)
  {
    return SerializeArrayImpl(stream, pArray, uiCount, 0);
  }
} // namespace xiiStreamWriterUtil

template <typename ArrayType, typename ValueType>
xiiResult xiiStreamWriter::WriteArray(const xiiArrayBase<ValueType, ArrayType>& Array)
{
  const xiiUInt64 uiCount = Array.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return xiiStreamWriterUtil::SerializeArray<ValueType>(*this, Array.GetArrayPtr().GetPtr(), Array.GetCount());
}

template <typename ValueType, xiiUInt32 uiSize>
xiiResult xiiStreamWriter::WriteArray(const ValueType (&Array)[uiSize])
{
  const xiiUInt64 uiWriteSize = uiSize;
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return xiiStreamWriterUtil::SerializeArray<ValueType>(*this, Array, uiSize);
}

template <typename KeyType, typename Comparer>
xiiResult xiiStreamWriter::WriteSet(const xiiSetBase<KeyType, Comparer>& Set)
{
  const xiiUInt64 uiWriteSize = Set.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : Set)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
xiiResult xiiStreamWriter::WriteMap(const xiiMapBase<KeyType, ValueType, Comparer>& Map)
{
  const xiiUInt64 uiWriteSize = Map.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = Map.GetIterator(); It.IsValid(); ++It)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return XII_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
xiiResult xiiStreamWriter::WriteHashTable(const xiiHashTableBase<KeyType, ValueType, Hasher>& HashTable)
{
  const xiiUInt64 uiWriteSize = HashTable.GetCount();
  XII_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = HashTable.GetIterator(); It.IsValid(); ++It)
  {
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    XII_SUCCEED_OR_RETURN(xiiStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return XII_SUCCESS;
}

namespace xiiStreamReaderUtil
{
  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& stream, T& Obj, int) -> decltype(stream >> Obj, xiiResult(XII_SUCCESS))
  {
    stream >> Obj;

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& stream, T& Obj, long) -> decltype(Obj.Deserialize(stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.Deserialize(stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto DeserializeImpl(xiiStreamReader& stream, T& Obj, float) -> decltype(Obj.deserialize(stream).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return xiiToResult(Obj.deserialize(stream));
  }

  template <class T>
  XII_ALWAYS_INLINE auto Deserialize(xiiStreamReader& stream, T& Obj) -> decltype(DeserializeImpl(stream, Obj, 0).IgnoreResult(), xiiResult(XII_SUCCESS))
  {
    return DeserializeImpl(stream, Obj, 0);
  }

  // serialization of array

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  template <class T>
  XII_ALWAYS_INLINE auto DeserializeArrayImpl(xiiStreamReader& stream, T* pArray, xiiUInt64 uiCount, int) -> decltype(DeserializeArray(stream, pArray, uiCount), xiiResult(XII_SUCCESS))
  {
    return DeserializeArray(stream, pArray, uiCount);
  }
#endif

  template <class T>
  xiiResult DeserializeArrayImpl(xiiStreamReader& stream, T* pArray, xiiUInt64 uiCount, long)
  {
    for (xiiUInt64 i = 0; i < uiCount; ++i)
    {
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize<T>(stream, pArray[i]));
    }

    return XII_SUCCESS;
  }

  template <class T>
  XII_ALWAYS_INLINE xiiResult DeserializeArray(xiiStreamReader& stream, T* pArray, xiiUInt64 uiCount)
  {
    return DeserializeArrayImpl(stream, pArray, uiCount, 0);
  }

} // namespace xiiStreamReaderUtil

template <typename ArrayType, typename ValueType>
xiiResult xiiStreamReader::ReadArray(xiiArrayBase<ValueType, ArrayType>& Array)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    Array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(Array).SetCount(static_cast<xiiUInt32>(uiCount));

      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::DeserializeArray<ValueType>(*this, Array.GetData(), uiCount));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}

template <typename ValueType, xiiUInt32 uiSize>
xiiResult xiiStreamReader::ReadArray(ValueType (&Array)[uiSize])
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
xiiResult xiiStreamReader::ReadSet(xiiSetBase<KeyType, Comparer>& Set)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    Set.Clear();

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType Item;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Item));

      Set.Insert(std::move(Item));
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
xiiResult xiiStreamReader::ReadMap(xiiMapBase<KeyType, ValueType, Comparer>& Map)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    Map.Clear();

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   Key;
      ValueType Value;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Key));
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Value));

      Map.Insert(std::move(Key), std::move(Value));
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
xiiResult xiiStreamReader::ReadHashTable(xiiHashTableBase<KeyType, ValueType, Hasher>& HashTable)
{
  xiiUInt64 uiCount = 0;
  XII_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < xiiMath::MaxValue<xiiUInt32>())
  {
    HashTable.Clear();
    HashTable.Reserve(static_cast<xiiUInt32>(uiCount));

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(uiCount); ++i)
    {
      KeyType   Key;
      ValueType Value;
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Key));
      XII_SUCCEED_OR_RETURN(xiiStreamReaderUtil::Deserialize(*this, Value));

      HashTable.Insert(std::move(Key), std::move(Value));
    }

    return XII_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return XII_FAILURE;
  }
}
