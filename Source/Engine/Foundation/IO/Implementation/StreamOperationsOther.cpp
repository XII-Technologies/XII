#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// xiiAllocatorBase::Stats

void operator<<(xiiStreamWriter& ref_stream, const xiiAllocatorBase::Stats& rhs)
{
  ref_stream << rhs.m_uiNumAllocations;
  ref_stream << rhs.m_uiNumDeallocations;
  ref_stream << rhs.m_uiAllocationSize;
}

void operator>>(xiiStreamReader& ref_stream, xiiAllocatorBase::Stats& rhs)
{
  ref_stream >> rhs.m_uiNumAllocations;
  ref_stream >> rhs.m_uiNumDeallocations;
  ref_stream >> rhs.m_uiAllocationSize;
}

// xiiTime

void operator<<(xiiStreamWriter& ref_stream, xiiTime value)
{
  ref_stream << value.GetSeconds();
}

void operator>>(xiiStreamReader& ref_stream, xiiTime& ref_value)
{
  double d = 0;
  ref_stream.ReadQWordValue(&d).IgnoreResult();

  ref_value = xiiTime::MakeFromSeconds(d);
}

// xiiUuid

void operator<<(xiiStreamWriter& ref_stream, const xiiUuid& value)
{
  ref_stream << value.m_uiHigh;
  ref_stream << value.m_uiLow;
}

void operator>>(xiiStreamReader& ref_stream, xiiUuid& ref_value)
{
  ref_stream >> ref_value.m_uiHigh;
  ref_stream >> ref_value.m_uiLow;
}

// xiiHashedString

void operator<<(xiiStreamWriter& ref_stream, const xiiHashedString& sValue)
{
  ref_stream.WriteString(sValue.GetView()).AssertSuccess();
}

void operator>>(xiiStreamReader& ref_stream, xiiHashedString& ref_sValue)
{
  xiiStringBuilder sTemp;
  ref_stream >> sTemp;
  ref_sValue.Assign(sTemp);
}

// xiiTempHashedString

void operator<<(xiiStreamWriter& ref_stream, const xiiTempHashedString& sValue)
{
  ref_stream << (xiiUInt64)sValue.GetHash();
}

void operator>>(xiiStreamReader& ref_stream, xiiTempHashedString& ref_sValue)
{
  xiiUInt64 hash;
  ref_stream >> hash;
  ref_sValue = xiiTempHashedString(hash);
}

// xiiVariant

struct WriteValueFunc
{
  template <typename T>
  XII_ALWAYS_INLINE void operator()()
  {
    (*m_pStream) << m_pValue->Get<T>();
  }

  xiiStreamWriter*  m_pStream;
  const xiiVariant* m_pValue;
};

template <>
XII_FORCE_INLINE void WriteValueFunc::operator()<xiiVariantArray>()
{
  const xiiVariantArray& values = m_pValue->Get<xiiVariantArray>();
  const xiiUInt32        iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) << values[i];
  }
}

template <>
XII_FORCE_INLINE void WriteValueFunc::operator()<xiiVariantDictionary>()
{
  const xiiVariantDictionary& values = m_pValue->Get<xiiVariantDictionary>();
  const xiiUInt32             iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    (*m_pStream) << it.Key();
    (*m_pStream) << it.Value();
  }
}

template <>
inline void WriteValueFunc::operator()<xiiTypedPointer>()
{
  XII_REPORT_FAILURE("Type 'xiiReflectedClass*' not supported in serialization.");
}

template <>
inline void WriteValueFunc::operator()<xiiTypedObject>()
{
  xiiTypedObject obj = m_pValue->Get<xiiTypedObject>();
  if (const xiiVariantTypeInfo* pTypeInfo = xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
  {
    (*m_pStream) << obj.m_pType->GetTypeName();
    pTypeInfo->Serialize(*m_pStream, obj.m_pObject);
  }
  else
  {
    XII_REPORT_FAILURE("The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
  }
}

template <>
XII_FORCE_INLINE void WriteValueFunc::operator()<xiiStringView>()
{
  xiiStringBuilder s = m_pValue->Get<xiiStringView>();
  (*m_pStream) << s;
}

template <>
XII_FORCE_INLINE void WriteValueFunc::operator()<xiiDataBuffer>()
{
  const xiiDataBuffer& data   = m_pValue->Get<xiiDataBuffer>();
  const xiiUInt32      iCount = data.GetCount();
  (*m_pStream) << iCount;
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).AssertSuccess();
}

struct ReadValueFunc
{
  template <typename T>
  XII_FORCE_INLINE void operator()()
  {
    T value;
    (*m_pStream) >> value;
    *m_pValue = value;
  }

  xiiStreamReader* m_pStream;
  xiiVariant*      m_pValue;
};

template <>
XII_FORCE_INLINE void ReadValueFunc::operator()<xiiVariantArray>()
{
  xiiVariantArray values;
  xiiUInt32       iCount;
  (*m_pStream) >> iCount;
  values.SetCount(iCount);
  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) >> values[i];
  }
  *m_pValue = values;
}

template <>
XII_FORCE_INLINE void ReadValueFunc::operator()<xiiVariantDictionary>()
{
  xiiVariantDictionary values;
  xiiUInt32            iCount;
  (*m_pStream) >> iCount;
  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    xiiString  key;
    xiiVariant value;
    (*m_pStream) >> key;
    (*m_pStream) >> value;
    values.Insert(key, value);
  }
  *m_pValue = values;
}

template <>
inline void ReadValueFunc::operator()<xiiTypedPointer>()
{
  XII_REPORT_FAILURE("Type 'xiiTypedPointer' not supported in serialization.");
}

template <>
inline void ReadValueFunc::operator()<xiiTypedObject>()
{
  xiiStringBuilder sType;
  (*m_pStream) >> sType;
  const xiiRTTI* pType = xiiRTTI::FindTypeByName(sType);
  XII_ASSERT_DEV(pType, "The type '{0}' could not be found.", sType);
  const xiiVariantTypeInfo* pTypeInfo = xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  XII_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", sType);
  XII_MSVC_ANALYSIS_ASSUME(pType != nullptr);
  XII_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  void* pObject = pType->GetAllocator()->Allocate<void>();
  pTypeInfo->Deserialize(*m_pStream, pObject);
  m_pValue->MoveTypedObject(pObject, pType);
}

template <>
inline void ReadValueFunc::operator()<xiiStringView>()
{
  XII_REPORT_FAILURE("Type 'xiiStringView' not supported in serialization.");
}

template <>
XII_FORCE_INLINE void ReadValueFunc::operator()<xiiDataBuffer>()
{
  xiiDataBuffer data;
  xiiUInt32     iCount;
  (*m_pStream) >> iCount;
  data.SetCountUninitialized(iCount);

  m_pStream->ReadBytes(data.GetData(), iCount);
  *m_pValue = data;
}

void operator<<(xiiStreamWriter& ref_stream, const xiiVariant& value)
{
  xiiUInt8 variantVersion = (xiiUInt8)xiiGetStaticRTTI<xiiVariant>()->GetTypeVersion();
  ref_stream << variantVersion;
  xiiVariant::Type::Enum type        = value.GetType();
  xiiUInt8               typeStorage = type;
  if (typeStorage == xiiVariantType::StringView)
    typeStorage = xiiVariantType::String;
  ref_stream << typeStorage;

  if (type != xiiVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &ref_stream;
    func.m_pValue  = &value;

    xiiVariant::DispatchTo(func, type);
  }
}

void operator>>(xiiStreamReader& ref_stream, xiiVariant& ref_value)
{
  xiiUInt8 variantVersion;
  ref_stream >> variantVersion;
  XII_ASSERT_DEBUG(xiiGetStaticRTTI<xiiVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  xiiUInt8 typeStorage;
  ref_stream >> typeStorage;
  xiiVariant::Type::Enum type = (xiiVariant::Type::Enum)typeStorage;

  if (type != xiiVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &ref_stream;
    func.m_pValue  = &ref_value;

    xiiVariant::DispatchTo(func, type);
  }
  else
  {
    ref_value = xiiVariant();
  }
}

// xiiTimestamp

void operator<<(xiiStreamWriter& ref_stream, xiiTimestamp value)
{
  ref_stream << value.GetInt64(xiiSIUnitOfTime::Microsecond);
}

void operator>>(xiiStreamReader& ref_stream, xiiTimestamp& ref_value)
{
  xiiInt64 value;
  ref_stream >> value;

  ref_value = xiiTimestamp::MakeFromInt(value, xiiSIUnitOfTime::Microsecond);
}

// xiiVarianceTypeFloat

void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeFloat& value)
{
  ref_stream << value.m_fVariance;
  ref_stream << value.m_Value;
}
void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeFloat& ref_value)
{
  ref_stream >> ref_value.m_fVariance;
  ref_stream >> ref_value.m_Value;
}

// xiiVarianceTypeDouble

void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeDouble& value)
{
  ref_stream << value.m_fVariance;
  ref_stream << value.m_Value;
}
void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeDouble& ref_value)
{
  ref_stream >> ref_value.m_fVariance;
  ref_stream >> ref_value.m_Value;
}

// xiiVarianceTypeTime

void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeTime& value)
{
  ref_stream << value.m_fVariance;
  ref_stream << value.m_Value;
}
void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeTime& ref_value)
{
  ref_stream >> ref_value.m_fVariance;
  ref_stream >> ref_value.m_Value;
}

// xiiVarianceTypeAngle

void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngle& value)
{
  ref_stream << value.m_fVariance;
  ref_stream << value.m_Value;
}
void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngle& ref_value)
{
  ref_stream >> ref_value.m_fVariance;
  ref_stream >> ref_value.m_Value;
}

// xiiVarianceTypeAngled

void operator<<(xiiStreamWriter& ref_stream, const xiiVarianceTypeAngled& value)
{
  ref_stream << value.m_fVariance;
  ref_stream << value.m_Value;
}
void operator>>(xiiStreamReader& ref_stream, xiiVarianceTypeAngled& ref_value)
{
  ref_stream >> ref_value.m_fVariance;
  ref_stream >> ref_value.m_Value;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);
