
#define XII_MSVC_WARNING_NUMBER 4702 // Unreachable code for some reason
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

XII_ALWAYS_INLINE xiiVariant::xiiVariant()
{
  m_uiType    = Type::Invalid;
  m_bIsShared = false;
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVariant& other)
{
  CopyFrom(other);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(xiiVariant&& other) noexcept
{
  MoveFrom(std::move(other));
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const bool& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiInt8& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiUInt8& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiInt16& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiUInt16& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiInt32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiUInt32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiInt64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiUInt64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const float& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const double& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiColor& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2d& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3d& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4d& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2I32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2I64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3I32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3I64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4I32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4I64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2U32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec2U64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3U32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec3U64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4U32& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiVec4U64& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiQuat& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiQuatd& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiTime& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiUuid& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiAngle& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiAngled& value)
{
  InitInplace(value);
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(const xiiColorGammaUB& value)
{
  InitInplace(value);
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, int>>
XII_ALWAYS_INLINE xiiVariant::xiiVariant(const T& value)
{
  const constexpr bool forceSharing = TypeDeduction<T>::forceSharing;
  const constexpr bool inlineSized  = sizeof(T) <= InlinedStruct::DataSize;
  const constexpr bool isPOD        = xiiIsPodType<T>::value;
  InitTypedObject(value, xiiTraitInt < (!forceSharing && inlineSized && isPOD) ? 1 : 0 > ());
}

template <typename T>
XII_ALWAYS_INLINE xiiVariant::xiiVariant(const T* value)
{
  constexpr bool bla = !std::is_same<T, void>::value;
  XII_CHECK_AT_COMPILETIME(bla);
  InitTypedPointer(const_cast<T*>(value), xiiGetStaticRTTI<T>());
}

XII_ALWAYS_INLINE xiiVariant::xiiVariant(void* value, const xiiRTTI* pType)
{
  InitTypedPointer(value, pType);
}

XII_ALWAYS_INLINE xiiVariant::~xiiVariant()
{
  Release();
}

XII_ALWAYS_INLINE void xiiVariant::operator=(const xiiVariant& other)
{
  if (this != &other)
  {
    Release();
    CopyFrom(other);
  }
}

XII_ALWAYS_INLINE void xiiVariant::operator=(xiiVariant&& other) noexcept
{
  if (this != &other)
  {
    Release();
    MoveFrom(std::move(other));
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiVariant::operator=(const T& value)
{
  *this = xiiVariant(value);
}

XII_ALWAYS_INLINE bool xiiVariant::operator!=(const xiiVariant& other) const
{
  return !(*this == other);
}

template <typename T>
XII_FORCE_INLINE bool xiiVariant::operator==(const T& other) const
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  struct TypeInfo
  {
    enum
    {
      isNumber = TypeDeduction<T>::value > Type::Invalid&& TypeDeduction<T>::value <= Type::Double
    };
  };

  if (IsFloatingPoint())
  {
    return xiiVariantHelper::CompareFloat(*this, other, xiiTraitInt<TypeInfo::isNumber>());
  }
  else if (IsNumber())
  {
    return xiiVariantHelper::CompareNumber(*this, other, xiiTraitInt<TypeInfo::isNumber>());
  }

  XII_ASSERT_DEV(IsA<StorageType>(), "Stored type '{0}' does not match comparison type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<StorageType>() == other;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiVariant::operator!=(const T& other) const
{
  return !(*this == other);
}

XII_ALWAYS_INLINE bool xiiVariant::IsValid() const
{
  return m_uiType != Type::Invalid;
}

XII_ALWAYS_INLINE bool xiiVariant::IsNumber() const
{
  return IsNumberStatic(m_uiType);
}

XII_ALWAYS_INLINE bool xiiVariant::IsFloatingPoint() const
{
  return IsFloatingPointStatic(m_uiType);
}

XII_ALWAYS_INLINE bool xiiVariant::IsString() const
{
  return IsStringStatic(m_uiType);
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, int>>
XII_ALWAYS_INLINE bool xiiVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, int>>
XII_ALWAYS_INLINE bool xiiVariant::IsA() const
{
  if (m_uiType == TypeDeduction<T>::value)
  {
    const xiiTypedPointer& ptr = *reinterpret_cast<const xiiTypedPointer*>(&m_Data);
    // Always allow cast to void*.
    if constexpr (std::is_same<T, void*>::value || std::is_same<T, const void*>::value)
    {
      return true;
    }
    else if (ptr.m_pType)
    {
      using NonPointerT    = typename xiiTypeTraits<T>::NonConstReferencePointerType;
      const xiiRTTI* pType = xiiGetStaticRTTI<NonPointerT>();
      return IsDerivedFrom(ptr.m_pType, pType);
    }
    else if (!ptr.m_pObject)
    {
      // nullptr can be converted to anything
      return true;
    }
  }
  return false;
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, int>>
XII_ALWAYS_INLINE bool xiiVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, int>>
XII_ALWAYS_INLINE bool xiiVariant::IsA() const
{
  using NonRefT = typename xiiTypeTraits<T>::NonConstReferenceType;
  if (m_uiType == TypeDeduction<T>::value)
  {
    if (const xiiRTTI* pType = GetReflectedType())
    {
      return IsDerivedFrom(pType, xiiGetStaticRTTI<NonRefT>());
    }
  }
  return false;
}

XII_ALWAYS_INLINE xiiVariant::Type::Enum xiiVariant::GetType() const
{
  return static_cast<Type::Enum>(m_uiType);
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, int>>
XII_ALWAYS_INLINE const T& xiiVariant::Get() const
{
  XII_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, int>>
XII_ALWAYS_INLINE T xiiVariant::Get() const
{
  XII_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, int>>
XII_ALWAYS_INLINE const T xiiVariant::Get() const
{
  XII_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, int>>
XII_ALWAYS_INLINE const T& xiiVariant::Get() const
{
  XII_ASSERT_DEV(m_uiType == TypeDeduction<T>::value, "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, int>>
XII_ALWAYS_INLINE T& xiiVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, int>>
XII_ALWAYS_INLINE T xiiVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T>(Get<T>());
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, int>>
XII_ALWAYS_INLINE T& xiiVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

XII_ALWAYS_INLINE const void* xiiVariant::GetData() const
{
  if (m_uiType == Type::TypedPointer)
  {
    return Cast<xiiTypedPointer>().m_pObject;
  }
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiVariant::CanConvertTo() const
{
  return CanConvertTo(static_cast<Type::Enum>(TypeDeduction<T>::value));
}

template <typename T>
T xiiVariant::ConvertTo(xiiResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo<T>())
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = XII_FAILURE;

    return T();
  }

  if (m_uiType == TypeDeduction<T>::value)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = XII_SUCCESS;

    return Cast<T>();
  }

  T    result;
  bool bSuccessful = true;
  xiiVariantHelper::To(*this, result, bSuccessful);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = bSuccessful ? XII_SUCCESS : XII_FAILURE;

  return result;
}


/// private methods

template <typename T>
XII_FORCE_INLINE void xiiVariant::InitInplace(const T& value)
{
  XII_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  XII_CHECK_AT_COMPILETIME_MSG(xiiIsPodType<T>::value, "in place data needs to be POD");
  xiiMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);

  m_uiType    = TypeDeduction<T>::value;
  m_bIsShared = false;
}

template <typename T>
XII_FORCE_INLINE void xiiVariant::InitTypedObject(const T& value, xiiTraitInt<0>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  XII_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(InlinedStruct::DataSize)) || TypeDeduction<T>::forceSharing, "Value should be inplace instead.");
  XII_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  const xiiRTTI* pType = xiiGetStaticRTTI<T>();
  m_Data.shared        = XII_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType             = Type::TypedObject;
  m_bIsShared          = true;
}

template <typename T>
XII_FORCE_INLINE void xiiVariant::InitTypedObject(const T& value, xiiTraitInt<1>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  XII_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) <= InlinedStruct::DataSize) && !TypeDeduction<T>::forceSharing, "Value can't be stored inplace.");
  XII_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  XII_CHECK_AT_COMPILETIME_MSG(xiiIsPodType<T>::value, "in place data needs to be POD");
  xiiMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_Data.inlined.m_pType = xiiGetStaticRTTI<T>();
  m_uiType               = Type::TypedObject;
  m_bIsShared            = false;
}

inline void xiiVariant::Release()
{
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef.Decrement() == 0)
    {
      XII_DEFAULT_DELETE(m_Data.shared);
    }
  }
}

inline void xiiVariant::CopyFrom(const xiiVariant& other)
{
  m_uiType    = other.m_uiType;
  m_bIsShared = other.m_bIsShared;

  if (m_bIsShared)
  {
    m_Data.shared = other.m_Data.shared;
    m_Data.shared->m_uiRef.Increment();
  }
  else if (other.IsValid())
  {
    m_Data = other.m_Data;
  }
}

XII_ALWAYS_INLINE void xiiVariant::MoveFrom(xiiVariant&& other)
{
  m_uiType    = other.m_uiType;
  m_bIsShared = other.m_bIsShared;
  m_Data      = other.m_Data;

  other.m_uiType      = Type::Invalid;
  other.m_bIsShared   = false;
  other.m_Data.shared = nullptr;
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, int>>
const T& xiiVariant::Cast() const
{
  const bool validType = xiiConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  XII_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, int>>
T xiiVariant::Cast() const
{
  const xiiTypedPointer& ptr = *reinterpret_cast<const xiiTypedPointer*>(&m_Data);

  const xiiRTTI* pType = GetReflectedType();
  using NonRefPtrT     = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  if constexpr (!std::is_same<T, void*>::value && !std::is_same<T, const void*>::value)
  {
    XII_ASSERT_DEV(pType == nullptr || IsDerivedFrom(pType, xiiGetStaticRTTI<NonRefPtrT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(xiiGetStaticRTTI<NonRefPtrT>()));
  }
  return static_cast<T>(ptr.m_pObject);
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, int>>
const T xiiVariant::Cast() const
{
  xiiTypedObject obj;
  obj.m_pObject = GetData();
  obj.m_pType   = GetReflectedType();
  return obj;
}

template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, int>>
const T& xiiVariant::Cast() const
{
  const xiiRTTI* pType = GetReflectedType();
  using NonRefT        = typename xiiTypeTraits<T>::NonConstReferenceType;
  XII_ASSERT_DEV(IsDerivedFrom(pType, xiiGetStaticRTTI<NonRefT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(xiiGetStaticRTTI<NonRefT>()));

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

XII_ALWAYS_INLINE bool xiiVariant::IsNumberStatic(xiiUInt32 type)
{
  return type > Type::FirstStandardType && type <= Type::Double;
}

XII_ALWAYS_INLINE bool xiiVariant::IsFloatingPointStatic(xiiUInt32 type)
{
  return type == Type::Float || type == Type::Double;
}

XII_ALWAYS_INLINE bool xiiVariant::IsStringStatic(xiiUInt32 type)
{
  return type == Type::String || type == Type::StringView;
}

XII_ALWAYS_INLINE bool xiiVariant::IsVector2Static(xiiUInt32 type)
{
  return type == Type::Vector2 || type == Type::Vector2d || type == Type::Vector2I || type == Type::Vector2I64 || type == Type::Vector2U || type == Type::Vector2U64;
}

XII_ALWAYS_INLINE bool xiiVariant::IsVector3Static(xiiUInt32 type)
{
  return type == Type::Vector3 || type == Type::Vector3d || type == Type::Vector3I || type == Type::Vector3I64 || type == Type::Vector3U || type == Type::Vector3U64;
}

XII_ALWAYS_INLINE bool xiiVariant::IsVector4Static(xiiUInt32 type)
{
  return type == Type::Vector4 || type == Type::Vector4d || type == Type::Vector4I || type == Type::Vector4I64 || type == Type::Vector4U || type == Type::Vector4U64;
}

XII_ALWAYS_INLINE bool xiiVariant::IsQuatStatic(xiiUInt32 type)
{
  return type == Type::Quaternion || type == Type::Quaterniond;
}

XII_ALWAYS_INLINE bool xiiVariant::IsMat3Static(xiiUInt32 type)
{
  return type == Type::Matrix3 || type == Type::Matrix3d;
}

XII_ALWAYS_INLINE bool xiiVariant::IsMat4Static(xiiUInt32 type)
{
  return type == Type::Matrix4 || type == Type::Matrix4d;
}

XII_ALWAYS_INLINE bool xiiVariant::IsTransformStatic(xiiUInt32 type)
{
  return type == Type::Transform || type == Type::Transformd;
}

template <typename T>
T xiiVariant::ConvertNumber() const
{
  switch (m_uiType)
  {
    case Type::Bool:
      return static_cast<T>(Cast<bool>());
    case Type::Int8:
      return static_cast<T>(Cast<xiiInt8>());
    case Type::UInt8:
      return static_cast<T>(Cast<xiiUInt8>());
    case Type::Int16:
      return static_cast<T>(Cast<xiiInt16>());
    case Type::UInt16:
      return static_cast<T>(Cast<xiiUInt16>());
    case Type::Int32:
      return static_cast<T>(Cast<xiiInt32>());
    case Type::UInt32:
      return static_cast<T>(Cast<xiiUInt32>());
    case Type::Int64:
      return static_cast<T>(Cast<xiiInt64>());
    case Type::UInt64:
      return static_cast<T>(Cast<xiiUInt64>());
    case Type::Float:
      return static_cast<T>(Cast<float>());
    case Type::Double:
      return static_cast<T>(Cast<double>());
  }

  XII_REPORT_FAILURE("Variant is not a number");
  return T(0);
}

template <>
struct xiiHashHelper<xiiVariant>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiVariant& value)
  {
    xiiUInt64 uiHash = value.ComputeHash(0);
    return (xiiUInt32)uiHash;
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiVariant& a, const xiiVariant& b) { return a == b; }
};
