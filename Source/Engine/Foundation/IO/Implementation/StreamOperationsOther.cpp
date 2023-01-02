#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// xiiAllocatorBase::Stats

void operator<<(xiiStreamWriter& Stream, const xiiAllocatorBase::Stats& rhs)
{
  Stream << rhs.m_uiNumAllocations;
  Stream << rhs.m_uiNumDeallocations;
  Stream << rhs.m_uiAllocationSize;
}

void operator>>(xiiStreamReader& Stream, xiiAllocatorBase::Stats& rhs)
{
  Stream >> rhs.m_uiNumAllocations;
  Stream >> rhs.m_uiNumDeallocations;
  Stream >> rhs.m_uiAllocationSize;
}

// xiiTime

void operator<<(xiiStreamWriter& Stream, xiiTime Value)
{
  Stream << Value.GetSeconds();
}

void operator>>(xiiStreamReader& Stream, xiiTime& Value)
{
  double d = 0;
  Stream.ReadQWordValue(&d).IgnoreResult();

  Value = xiiTime::Seconds(d);
}

// xiiUuid

void operator<<(xiiStreamWriter& Stream, const xiiUuid& Value)
{
  Stream << Value.m_uiHigh;
  Stream << Value.m_uiLow;
}

void operator>>(xiiStreamReader& Stream, xiiUuid& Value)
{
  Stream >> Value.m_uiHigh;
  Stream >> Value.m_uiLow;
}

// xiiHashedString

void operator<<(xiiStreamWriter& Stream, const xiiHashedString& Value)
{
  Stream.WriteString(Value.GetView()).IgnoreResult();
}

void operator>>(xiiStreamReader& Stream, xiiHashedString& Value)
{
  xiiStringBuilder sTemp;
  Stream >> sTemp;
  Value.Assign(sTemp);
}

// xiiTempHashedString

void operator<<(xiiStreamWriter& Stream, const xiiTempHashedString& Value)
{
  Stream << (xiiUInt64)Value.GetHash();
}

void operator>>(xiiStreamReader& Stream, xiiTempHashedString& Value)
{
  xiiUInt64 hash;
  Stream >> hash;
  Value = xiiTempHashedString(hash);
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
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).IgnoreResult();
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

void operator<<(xiiStreamWriter& Stream, const xiiVariant& Value)
{
  xiiUInt8 variantVersion = (xiiUInt8)xiiGetStaticRTTI<xiiVariant>()->GetTypeVersion();
  Stream << variantVersion;
  xiiVariant::Type::Enum type        = Value.GetType();
  xiiUInt8               typeStorage = type;
  if (typeStorage == xiiVariantType::StringView)
    typeStorage = xiiVariantType::String;
  Stream << typeStorage;

  if (type != xiiVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &Stream;
    func.m_pValue  = &Value;

    xiiVariant::DispatchTo(func, type);
  }
}

void operator>>(xiiStreamReader& Stream, xiiVariant& Value)
{
  xiiUInt8 variantVersion;
  Stream >> variantVersion;
  XII_ASSERT_DEBUG(xiiGetStaticRTTI<xiiVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  xiiUInt8 typeStorage;
  Stream >> typeStorage;
  xiiVariant::Type::Enum type = (xiiVariant::Type::Enum)typeStorage;

  if (type != xiiVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &Stream;
    func.m_pValue  = &Value;

    xiiVariant::DispatchTo(func, type);
  }
  else
  {
    Value = xiiVariant();
  }
}

// xiiTimestamp

void operator<<(xiiStreamWriter& Stream, xiiTimestamp Value)
{
  Stream << Value.GetInt64(xiiSIUnitOfTime::Microsecond);
}

void operator>>(xiiStreamReader& Stream, xiiTimestamp& Value)
{
  xiiInt64 value;
  Stream >> value;

  Value.SetInt64(value, xiiSIUnitOfTime::Microsecond);
}

// xiiVarianceTypeFloat

void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeFloat& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(xiiStreamReader& Stream, xiiVarianceTypeFloat& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}

// xiiVarianceTypeDouble

void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeDouble& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(xiiStreamReader& Stream, xiiVarianceTypeDouble& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}

// xiiVarianceTypeTime

void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeTime& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(xiiStreamReader& Stream, xiiVarianceTypeTime& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}

// xiiVarianceTypeAngle

void operator<<(xiiStreamWriter& Stream, const xiiVarianceTypeAngle& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(xiiStreamReader& Stream, xiiVarianceTypeAngle& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}
XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);
