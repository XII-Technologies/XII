#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/VariantTypeRegistry.h>

#if XII_ENABLED(XII_PLATFORM_64BIT)
XII_CHECK_AT_COMPILETIME(sizeof(xiiVariant) == 40);
#else
XII_CHECK_AT_COMPILETIME(sizeof(xiiVariant) == 40); // \todo Resolve
#endif

/// constructors

xiiVariant::xiiVariant(const xiiMat3& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiMat3d& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiMat4& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiMat4d& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiTransform& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiTransformd& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const char* value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiString& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiUntrackedString& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiDataBuffer& value)
{
  InitShared(value);
}

xiiVariant::xiiVariant(const xiiVariantArray& value)
{
  using StorageType = typename TypeDeduction<xiiVariantArray>::StorageType;

  m_Data.shared = XII_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType      = TypeDeduction<xiiVariantArray>::value;
  m_bIsShared   = true;
}

xiiVariant::xiiVariant(const xiiVariantDictionary& value)
{
  using StorageType = typename TypeDeduction<xiiVariantDictionary>::StorageType;

  m_Data.shared = XII_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType      = TypeDeduction<xiiVariantDictionary>::value;
  m_bIsShared   = true;
}

xiiVariant::xiiVariant(const xiiTypedPointer& value)
{
  InitInplace(value);
}

xiiVariant::xiiVariant(const xiiTypedObject& value)
{
  void* ptr     = xiiReflectionSerializer::Clone(value.m_pObject, value.m_pType);
  m_Data.shared = XII_DEFAULT_NEW(RTTISharedData, ptr, value.m_pType);
  m_uiType      = Type::TypedObject;
  m_bIsShared   = true;
}

void xiiVariant::CopyTypedObject(const void* value, const xiiRTTI* pType)
{
  Release();
  void* ptr     = xiiReflectionSerializer::Clone(value, pType);
  m_Data.shared = XII_DEFAULT_NEW(RTTISharedData, ptr, pType);
  m_uiType      = Type::TypedObject;
  m_bIsShared   = true;
}

void xiiVariant::MoveTypedObject(void* value, const xiiRTTI* pType)
{
  Release();
  m_Data.shared = XII_DEFAULT_NEW(RTTISharedData, value, pType);
  m_uiType      = Type::TypedObject;
  m_bIsShared   = true;
}

template <typename T>
XII_ALWAYS_INLINE void xiiVariant::InitShared(const T& value)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  XII_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  XII_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  const xiiRTTI* pType = xiiGetStaticRTTI<T>();

  m_Data.shared = XII_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType      = TypeDeduction<T>::value;
  m_bIsShared   = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  XII_FORCE_INLINE xiiUInt64 operator()(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
  {
    XII_CHECK_AT_COMPILETIME_MSG(sizeof(typename xiiVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                                   !xiiVariant::TypeDeduction<T>::forceSharing,
                                 "This type requires special handling! Add a specialization below.");
    return xiiHashingUtils::xxHash64(pData, sizeof(T), uiSeed);
  }
};


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec2I64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec2I64), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec3I64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec3I64), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec4I64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec4I64), uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec2U64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec2U64), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec3U64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec3U64), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec4U64>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec4U64), uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec2d>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec2d), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec3d>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec3d), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVec4d>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiVec4d), uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiString>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  auto pString = static_cast<const xiiString*>(pData);

  return xiiHashingUtils::xxHash64String(*pString, uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiQuatd>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiQuatd), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiMat3>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiMat3), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiMat3d>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiMat3d), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiMat4>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiMat4), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiMat4d>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiMat4d), uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiTransform>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiTransform), uiSeed);
}

template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiTransformd>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  return xiiHashingUtils::xxHash64(pData, sizeof(xiiTransformd), uiSeed);
}


template <>
XII_ALWAYS_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiDataBuffer>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  auto pDataBuffer = static_cast<const xiiDataBuffer*>(pData);

  return xiiHashingUtils::xxHash64(pDataBuffer->GetData(), pDataBuffer->GetCount(), uiSeed);
}


template <>
XII_FORCE_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiVariantArray>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  auto pVariantArray = static_cast<const xiiVariantArray*>(pData);

  xiiUInt64 uiHash = uiSeed;
  for (const xiiVariant& var : *pVariantArray)
  {
    uiHash = var.ComputeHash(uiHash);
  }

  return uiHash;
}

template <>
xiiUInt64 ComputeHashFunc::operator()<xiiVariantDictionary>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  auto pVariantDictionary = static_cast<const xiiVariantDictionary*>(pData);

  xiiHybridArray<xiiUInt64, 128> hashes;
  hashes.Reserve(pVariantDictionary->GetCount() * 2);

  for (auto& it : *pVariantDictionary)
  {
    hashes.PushBack(xiiHashingUtils::xxHash64String(it.Key(), uiSeed));
    hashes.PushBack(it.Value().ComputeHash(uiSeed));
  }

  hashes.Sort();

  return xiiHashingUtils::xxHash64(hashes.GetData(), hashes.GetCount() * sizeof(xiiUInt64), uiSeed);
}


template <>
XII_FORCE_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiTypedPointer>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  XII_IGNORE_UNUSED(pData);

  XII_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
XII_FORCE_INLINE xiiUInt64 ComputeHashFunc::operator()<xiiTypedObject>(const xiiVariant& v, const void* pData, xiiUInt64 uiSeed)
{
  auto pType = v.GetReflectedType();

  const xiiVariantTypeInfo* pTypeInfo = xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  XII_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", pType->GetTypeName());
  XII_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  xiiUInt32 uiHash32 = pTypeInfo->Hash(pData);

  return xiiHashingUtils::xxHash64(&uiHash32, sizeof(xiiUInt32), uiSeed);
}


struct CompareFunc
{
  template <typename T>
  XII_ALWAYS_INLINE void operator()()
  {
    m_bResult = m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const xiiVariant* m_pThis;
  const xiiVariant* m_pOther;
  bool              m_bResult;
};

template <>
XII_FORCE_INLINE void CompareFunc::operator()<xiiTypedObject>()
{
  m_bResult        = false;
  xiiTypedObject A = m_pThis->Get<xiiTypedObject>();
  xiiTypedObject B = m_pOther->Get<xiiTypedObject>();
  if (A.m_pType == B.m_pType)
  {
    const xiiVariantTypeInfo* pTypeInfo = xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(A.m_pType);
    XII_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", A.m_pType->GetTypeName());
    XII_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
    m_bResult = pTypeInfo->Equal(A.m_pObject, B.m_pObject);
  }
}

struct IndexFunc
{
  template <typename T>
  XII_FORCE_INLINE xiiVariant Impl(xiiTraitInt<1>)
  {
    const xiiRTTI*             pRtti = m_pThis->GetReflectedType();
    xiiAbstractMemberProperty* pProp = xiiReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
    if (!pProp)
      return xiiVariant();

    if (m_pThis->GetType() == xiiVariantType::TypedPointer)
    {
      const xiiTypedPointer& ptr = m_pThis->Get<xiiTypedPointer>();
      if (ptr.m_pObject)
        return xiiReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return xiiVariant();
    }
    return xiiReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  XII_ALWAYS_INLINE xiiVariant Impl(xiiTraitInt<0>)
  {
    return xiiVariant();
  }

  template <typename T>
  XII_FORCE_INLINE void operator()()
  {
    m_Result = Impl<T>(xiiTraitInt<xiiVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const xiiVariant* m_pThis;
  xiiVariant        m_Result;
  xiiUInt32         m_uiIndex;
};

struct KeyFunc
{
  template <typename T>
  XII_FORCE_INLINE xiiVariant Impl(xiiTraitInt<1>)
  {
    const xiiRTTI*             pRtti = m_pThis->GetReflectedType();
    xiiAbstractMemberProperty* pProp = xiiReflectionUtils::GetMemberProperty(pRtti, m_szKey);
    if (!pProp)
      return xiiVariant();
    if (m_pThis->GetType() == xiiVariantType::TypedPointer)
    {
      const xiiTypedPointer& ptr = m_pThis->Get<xiiTypedPointer>();
      if (ptr.m_pObject)
        return xiiReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return xiiVariant();
    }
    return xiiReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  XII_ALWAYS_INLINE xiiVariant Impl(xiiTraitInt<0>)
  {
    return xiiVariant();
  }

  template <typename T>
  XII_ALWAYS_INLINE void operator()()
  {
    m_Result = Impl<T>(xiiTraitInt<xiiVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const xiiVariant* m_pThis;
  xiiVariant        m_Result;
  const char*       m_szKey;
};

struct ConvertFunc
{
  template <typename T>
  XII_ALWAYS_INLINE void operator()()
  {
    T result;
    xiiVariantHelper::To(*m_pThis, result, m_bSuccessful);
    m_Result = result;
  }

  const xiiVariant* m_pThis;
  xiiVariant        m_Result;
  bool              m_bSuccessful;
};

/// public methods

bool xiiVariant::operator==(const xiiVariant& other) const
{
  if (m_uiType == Type::Invalid && other.m_uiType == Type::Invalid)
  {
    return true;
  }
  else if ((IsFloatingPoint() && other.IsNumber()) || (other.IsFloatingPoint() && IsNumber()))
  {
    // if either of them is a floating point number, compare them as doubles

    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber() && other.IsNumber())
  {
    return ConvertNumber<xiiInt64>() == other.ConvertNumber<xiiInt64>();
  }
  else if (IsString() && other.IsString())
  {
    const xiiStringView a = IsA<xiiStringView>() ? Get<xiiStringView>() : xiiStringView(Get<xiiString>().GetData());
    const xiiStringView b = other.IsA<xiiStringView>() ? other.Get<xiiStringView>() : xiiStringView(other.Get<xiiString>().GetData());
    return a.IsEqual(b);
  }
  else if (m_uiType == other.m_uiType)
  {
    CompareFunc compareFunc;
    compareFunc.m_pThis  = this;
    compareFunc.m_pOther = &other;

    DispatchTo(compareFunc, GetType());

    return compareFunc.m_bResult;
  }

  return false;
}

xiiTypedPointer xiiVariant::GetWriteAccess()
{
  xiiTypedPointer obj;
  obj.m_pType = GetReflectedType();
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef > 1)
    {
      // We need to make sure we hold the only reference to the shared data to be able to edit it.
      SharedData* pData = m_Data.shared->Clone();
      Release();
      m_Data.shared = pData;
    }
    obj.m_pObject = m_Data.shared->m_Ptr;
  }
  else
  {
    obj.m_pObject = m_uiType == Type::TypedPointer ? Cast<xiiTypedPointer>().m_pObject : &m_Data;
  }
  return obj;
}

const xiiVariant xiiVariant::operator[](xiiUInt32 uiIndex) const
{
  if (m_uiType == Type::VariantArray)
  {
    const xiiVariantArray& a = Cast<xiiVariantArray>();
    if (uiIndex < a.GetCount())
      return a[uiIndex];
  }
  else if (IsValid())
  {
    IndexFunc func;
    func.m_pThis   = this;
    func.m_uiIndex = uiIndex;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return xiiVariant();
}

const xiiVariant xiiVariant::operator[](StringWrapper key) const
{
  if (m_uiType == Type::VariantDictionary)
  {
    xiiVariant result;
    Cast<xiiVariantDictionary>().TryGetValue(key.m_str, result);
    return result;
  }
  else if (IsValid())
  {
    KeyFunc func;
    func.m_pThis = this;
    func.m_szKey = key.m_str;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return xiiVariant();
}

bool xiiVariant::CanConvertTo(Type::Enum type) const
{
  if (m_uiType == type)
    return true;

  if (type == Type::Invalid)
    return false;

  if (type == Type::String && (m_uiType < Type::LastStandardType && m_uiType != Type::DataBuffer))
    return true;
  if (type == Type::String && (m_uiType == Type::VariantArray || m_uiType == Type::VariantDictionary))
    return true;

  if (!IsValid())
    return false;

  if (IsNumberStatic(type) && (IsNumber() || m_uiType == Type::String))
    return true;

  if (IsVector2Static(type) && (IsVector2Static(m_uiType)))
    return true;

  if (IsVector3Static(type) && (IsVector3Static(m_uiType)))
    return true;

  if (IsVector4Static(type) && (IsVector4Static(m_uiType)))
    return true;

  if (IsQuatStatic(type) && (IsQuatStatic(m_uiType)))
    return true;

  if (IsMat3Static(type) && (IsMat3Static(m_uiType)))
    return true;

  if (IsMat4Static(type) && (IsMat4Static(m_uiType)))
    return true;

  if (IsTransformStatic(type) && (IsTransformStatic(m_uiType)))
    return true;

  if (type == Type::Color && m_uiType == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_uiType == Type::Color)
    return true;

  return false;
}

xiiVariant xiiVariant::ConvertTo(Type::Enum type, xiiResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo(type))
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = XII_FAILURE;

    return xiiVariant(); // creates an invalid variant
  }

  if (m_uiType == type)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = XII_SUCCESS;

    return *this;
  }

  ConvertFunc convertFunc;
  convertFunc.m_pThis       = this;
  convertFunc.m_bSuccessful = true;

  DispatchTo(convertFunc, type);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = convertFunc.m_bSuccessful ? XII_SUCCESS : XII_FAILURE;

  return convertFunc.m_Result;
}

xiiUInt64 xiiVariant::ComputeHash(xiiUInt64 uiSeed) const
{
  if (!IsValid())
    return uiSeed;

  ComputeHashFunc obj;
  return DispatchTo<ComputeHashFunc>(obj, GetType(), *this, GetData(), uiSeed);
}


inline xiiVariant::RTTISharedData::RTTISharedData(void* pData, const xiiRTTI* pType) :
  SharedData(pData, pType)
{
  XII_ASSERT_DEBUG(pType != nullptr && pType->GetAllocator()->CanAllocate(), "");
}

inline xiiVariant::RTTISharedData::~RTTISharedData()
{
  m_pType->GetAllocator()->Deallocate(m_Ptr);
}


xiiVariant::xiiVariant::SharedData* xiiVariant::RTTISharedData::Clone() const
{
  void* ptr = xiiReflectionSerializer::Clone(m_Ptr, m_pType);
  return XII_DEFAULT_NEW(RTTISharedData, ptr, m_pType);
}

struct GetTypeFromVariantFunc
{
  template <typename T>
  XII_ALWAYS_INLINE void operator()()
  {
    m_pType = xiiGetStaticRTTI<T>();
  }

  const xiiVariant* m_pVariant;
  const xiiRTTI*    m_pType;
};

template <>
XII_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<xiiVariantArray>()
{
  m_pType = nullptr;
}
template <>
XII_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<xiiVariantDictionary>()
{
  m_pType = nullptr;
}
template <>
XII_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<xiiTypedPointer>()
{
  m_pType = m_pVariant->Cast<xiiTypedPointer>().m_pType;
}
template <>
XII_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<xiiTypedObject>()
{
  m_pType = m_pVariant->m_bIsShared ? m_pVariant->m_Data.shared->m_pType : m_pVariant->m_Data.inlined.m_pType;
}

const xiiRTTI* xiiVariant::GetReflectedType() const
{
  if (m_uiType != Type::Invalid)
  {
    GetTypeFromVariantFunc func;
    func.m_pVariant = this;
    func.m_pType    = nullptr;
    xiiVariant::DispatchTo(func, GetType());
    return func.m_pType;
  }
  return nullptr;
}

void xiiVariant::InitTypedPointer(void* value, const xiiRTTI* pType)
{
  xiiTypedPointer ptr;
  ptr.m_pObject = value;
  ptr.m_pType   = pType;

  xiiMemoryUtils::CopyConstruct(reinterpret_cast<xiiTypedPointer*>(&m_Data), ptr, 1);

  m_uiType    = TypeDeduction<xiiTypedPointer>::value;
  m_bIsShared = false;
}

bool xiiVariant::IsDerivedFrom(const xiiRTTI* pType1, const xiiRTTI* pType2)
{
  return pType1->IsDerivedFrom(pType2);
}

const char* xiiVariant::GetTypeName(const xiiRTTI* pType)
{
  return pType->GetTypeName();
}

XII_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Variant);
