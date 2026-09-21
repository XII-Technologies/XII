/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Types/TypedPointer.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/VariantType.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Utilities/ConversionUtils.h>

class xiiRTTI;

/// Defines a reference to an immutable object owned by a xiiVariant.
///
/// Used to store custom types inside a xiiVariant. As lifetime is governed by the xiiVariant, it is generally not safe to store a xiiTypedObject.
/// This class is needed to be able to differentiate between xiiVariantType::TypedPointer and xiiVariantType::TypedObject e.g. in xiiVariant::DispatchTo.
/// \sa xiiVariant, XII_DECLARE_CUSTOM_VARIANT_TYPE
struct xiiTypedObject
{
  XII_DECLARE_POD_TYPE();

  const void*    m_pObject = nullptr;
  const xiiRTTI* m_pType   = nullptr;

  bool operator==(const xiiTypedObject& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
};

/// xiiVariant is a class that can store different types of variables, which is useful in situations where it is not clear up front,
/// which type of data will be passed around.
///
/// The variant supports a fixed list of types that it can store (\see xiiVariant::Type). All types of 16 bytes or less in size can be stored
/// without requiring a heap allocation. For larger types memory is allocated on the heap. In general variants should be used for code that
/// needs to be flexible. Although xiiVariant is implemented very efficiently, it should be avoided to use xiiVariant in code that needs to be
/// fast.
class XII_FOUNDATION_DLL xiiVariant
{
public:
  using Type = xiiVariantType;
  template <typename T>
  using TypeDeduction = xiiVariantTypeDeduction<T>;

  /// helper struct to wrap a string pointer.
  struct StringWrapper
  {
    XII_ALWAYS_INLINE StringWrapper(const char* szStr) :
      m_szStr(szStr)
    {
    }
    const char* m_szStr;
  };

  /// Initializes the variant to be 'Invalid'
  xiiVariant(); // [tested]

  /// Copies the data from the other variant.
  ///
  /// \note If the data of the variant needed to be allocated on the heap, it will be shared among variants.
  /// Thus, once you have stored such a type inside a variant, you can copy it to other variants, without introducing
  /// additional memory allocations.
  xiiVariant(const xiiVariant& other); // [tested]

  /// Moves the data from the other variant.
  xiiVariant(xiiVariant&& other) noexcept; // [tested]

  xiiVariant(const bool& value);
  xiiVariant(const xiiInt8& value);
  xiiVariant(const xiiUInt8& value);
  xiiVariant(const xiiInt16& value);
  xiiVariant(const xiiUInt16& value);
  xiiVariant(const xiiInt32& value);
  xiiVariant(const xiiUInt32& value);
  xiiVariant(const xiiInt64& value);
  xiiVariant(const xiiUInt64& value);
  xiiVariant(const float& value);
  xiiVariant(const double& value);
  xiiVariant(const xiiColor& value);
  xiiVariant(const xiiVec2& value);
  xiiVariant(const xiiVec2d& value);
  xiiVariant(const xiiVec3& value);
  xiiVariant(const xiiVec3d& value);
  xiiVariant(const xiiVec4& value);
  xiiVariant(const xiiVec4d& value);
  xiiVariant(const xiiVec2I32& value);
  xiiVariant(const xiiVec2I64& value);
  xiiVariant(const xiiVec3I32& value);
  xiiVariant(const xiiVec3I64& value);
  xiiVariant(const xiiVec4I32& value);
  xiiVariant(const xiiVec4I64& value);
  xiiVariant(const xiiVec2U32& value);
  xiiVariant(const xiiVec2U64& value);
  xiiVariant(const xiiVec3U32& value);
  xiiVariant(const xiiVec3U64& value);
  xiiVariant(const xiiVec4U32& value);
  xiiVariant(const xiiVec4U64& value);
  xiiVariant(const xiiQuat& value);
  xiiVariant(const xiiQuatd& value);
  xiiVariant(const xiiMat3& value);
  xiiVariant(const xiiMat3d& value);
  xiiVariant(const xiiMat4& value);
  xiiVariant(const xiiMat4d& value);
  xiiVariant(const xiiTransform& value);
  xiiVariant(const xiiTransformd& value);
  xiiVariant(const char* value);
  xiiVariant(const xiiString& value);
  xiiVariant(const xiiUntrackedString& value);
  xiiVariant(const xiiStringView& value, bool bCopyString = true);
  xiiVariant(const xiiHashedString& value);
  xiiVariant(const xiiTempHashedString& value);
  xiiVariant(const xiiDataBuffer& value);
  xiiVariant(const xiiTime& value);
  xiiVariant(const xiiUuid& value);
  xiiVariant(const xiiAngle& value);
  xiiVariant(const xiiAngled& value);
  xiiVariant(const xiiColorGammaUB& value);

  xiiVariant(const xiiVariantArray& value);
  xiiVariant(const xiiVariantDictionary& value);

  xiiVariant(const xiiTypedPointer& value);
  xiiVariant(const xiiTypedObject& value);

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, xiiInt32> = 0>
  xiiVariant(const T& value);

  template <typename T>
  xiiVariant(const T* value);

  /// Initializes to a TypedPointer of the given object and type.
  xiiVariant(void* value, const xiiRTTI* pType);

  /// Initializes to a TypedObject by cloning the given object and type.
  void CopyTypedObject(const void* value, const xiiRTTI* pType); // [tested]

  /// Initializes to a TypedObject by taking ownership of the given object and type.
  void MoveTypedObject(void* value, const xiiRTTI* pType); // [tested]

  /// If necessary, this will deallocate any heap memory that is not in use any more.
  ~xiiVariant();

  /// Copies the data from the \a other variant into this one.
  void operator=(const xiiVariant& other); // [tested]

  /// Moves the data from the \a other variant into this one.
  void operator=(xiiVariant&& other) noexcept; // [tested]

  /// Deduces the type of \a T and stores \a value.
  ///
  /// If the type to be stored in the variant is not supported, a compile time error will occur.
  template <typename T>
  void operator=(const T& value); // [tested]

  /// Will compare the value of this variant to that of \a other.
  ///
  /// If both variants store 'numbers' (float, double, xiiInt32 types) the comparison will work, even if the types are not identical.
  ///
  /// \note If the two types are not numbers and not equal, an assert will occur. So be careful to only compare variants
  /// that can either both be converted to double (\see CanConvertTo()) or whose types are equal.
  bool operator==(const xiiVariant& other) const; // [tested]

  /// See non-templated operator==
  template <typename T>
  bool operator==(const T& other) const; // [tested]

  /// Returns whether this variant stores any other type than 'Invalid'.
  bool IsValid() const; // [tested]

  /// Returns whether the stored type is numerical type either integer or floating point.
  ///
  /// Bool counts as number.
  bool IsNumber() const; // [tested]

  /// Returns whether the stored type is floating point (float or double).
  bool IsFloatingPoint() const; // [tested]

  /// Returns whether the stored type is a string (xiiString or xiiStringView).
  bool IsString() const; // [tested]

  /// Returns whether the stored type is a hashed string (xiiHashedString or xiiTempHashedString).
  bool IsHashedString() const;

  /// Returns whether the stored type is exactly the given type.
  ///
  /// \note This explicitly also differentiates between the different integer types.
  /// So when the variant stores an Int32, IsA<Int64>() will return false, even though the types could be converted.
  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, xiiInt32> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, xiiInt32> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, xiiInt32> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, xiiInt32> = 0>
  bool IsA() const; // [tested]

  /// Returns the exact xiiVariant::Type value.
  Type::Enum GetType() const; // [tested]

  /// Returns the variants value as the provided type.
  ///
  /// \note This function does not do ANY type of conversion from the stored type to the given type. Not even integer conversions!
  /// If the types don't match, this function will assert!
  /// So be careful to use this function only when you know exactly that the stored type matches the expected type.
  ///
  /// Prefer to use ConvertTo() when you can instead.
  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, xiiInt32> = 0>
  const T& Get() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, xiiInt32> = 0>
  T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, xiiInt32> = 0>
  const T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, xiiInt32> = 0>
  const T& Get() const; // [tested]

  /// Returns an writable xiiTypedPointer to the internal data.
  /// If the data is currently shared a clone will be made to ensure we hold the only reference.
  xiiTypedPointer GetWriteAccess(); // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, xiiInt32> = 0>
  T& GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, xiiInt32> = 0>
  T GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, xiiInt32> = 0>
  T& GetWritable(); // [tested]


  /// Returns a const void* to the internal data.
  /// For TypedPointer and TypedObject this will return a pointer to the target object.
  const void* GetData() const; // [tested]

  /// Returns the xiiRTTI type of the held value.
  /// For TypedPointer and TypedObject this will return the type of the target object.
  const xiiRTTI* GetReflectedType() const; // [tested]

  /// Returns the sub value at iIndex. This could be an element in an array or a member property inside a reflected type.
  ///
  /// Out of bounds access is handled gracefully and will return an invalid variant.
  const xiiVariant operator[](xiiUInt32 uiIndex) const; // [tested]

  /// Returns the sub value with szKey. This could be a value in a dictionary or a member property inside a reflected type.
  ///
  /// This function will return an invalid variant if no corresponding sub value is found.
  const xiiVariant operator[](StringWrapper key) const; // [tested]

  /// Returns whether the stored type can generally be converted to the desired type.
  ///
  /// This function will return true for all number conversions, as float / double / int / etc. can generally be converted into each
  /// other. It will also return true for all conversion from string to number types, and from all 'simple' types (not array or dictionary)
  /// to string.
  ///
  /// \note This function only returns whether a conversion between the stored TYPE and the desired TYPE is generally possible. It does NOT
  /// return whether the stored VALUE is indeed convertible to the desired type. For example, a string is generally convertible to float, if
  /// it stores a string representation of a float value. If, however, it stores anything else, the conversion can still fail.
  ///
  /// The only way to figure out whether the stored data can be converted to some type, is to actually convert it, using ConvertTo(), and
  /// then to check the conversion status.
  template <typename T>
  bool CanConvertTo() const; // [tested]

  /// Same as the templated CanConvertTo function.
  bool CanConvertTo(Type::Enum type) const; // [tested]

  /// Tries to convert the stored value to the given type. The optional status parameter can be used to check whether the conversion
  /// succeeded.
  ///
  /// When CanConvertTo() returns false, ConvertTo() will also always fail. However, when CanConvertTo() returns true, this is no guarantee
  /// that ConvertTo() will succeed. Conversion between numbers and to strings will generally succeed. However, converting from a string to
  /// another type can fail or succeed, depending on the exact string value.
  template <typename T>
  T ConvertTo(xiiResult* out_pConversionStatus = nullptr) const; // [tested]

  /// Same as the templated function.
  xiiVariant ConvertTo(Type::Enum type, xiiResult* out_pConversionStatus = nullptr) const; // [tested]

  /// This will call the overloaded operator() (function call operator) of the provided functor.
  ///
  /// This allows to implement a functor that overloads operator() for different types and then call the proper version of that operator,
  /// depending on the provided runtime type. Note that the proper overload of operator() is selected by providing a dummy type, but it will
  /// contain no useful value. Instead, store the other necessary data inside the functor object, before calling this function. For example,
  /// store a pointer to a variant inside the functor object and then call DispatchTo to execute the function that will handle the given
  /// type of the variant.
  template <typename Functor, class... Args>
  static auto DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args); // [tested]

  /// Computes the hash value of the stored data. Returns uiSeed (unchanged) for an invalid Variant.
  xiiUInt64 ComputeHash(xiiUInt64 uiSeed = 0) const;

private:
  friend class xiiVariantHelper;
  friend struct CompareFunc;
  friend struct GetTypeFromVariantFunc;

  struct SharedData
  {
    void*              m_Ptr;
    const xiiRTTI*     m_pType;
    xiiAtomicInteger32 m_uiRef = 1;

    XII_ALWAYS_INLINE SharedData(void* pPtr, const xiiRTTI* pType) :
      m_Ptr(pPtr), m_pType(pType)
    {
    }
    virtual ~SharedData()             = default;
    virtual SharedData* Clone() const = 0;
  };

  template <typename T>
  class TypedSharedData : public SharedData
  {
  private:
    T m_t;

  public:
    XII_ALWAYS_INLINE TypedSharedData(const T& value, const xiiRTTI* pType = nullptr) :
      SharedData(&m_t, pType), m_t(value)
    {
    }

    virtual SharedData* Clone() const override
    {
      return XII_DEFAULT_NEW(TypedSharedData<T>, m_t, m_pType);
    }
  };

  class RTTISharedData : public SharedData
  {
  public:
    RTTISharedData(void* pData, const xiiRTTI* pType);

    ~RTTISharedData();

    virtual SharedData* Clone() const override;
  };

  struct InlinedStruct
  {
    constexpr static xiiInt32 DataSize = 4 * sizeof(float) - sizeof(void*);
    xiiUInt8                  m_Data[DataSize];
    const xiiRTTI*            m_pType;
  };

  union Data
  {
    float         f[4];
    double        d[4];
    SharedData*   shared;
    InlinedStruct inlined;
  } m_Data;

  xiiUInt32 m_uiType : 31;
  xiiUInt32 m_bIsShared : 1; // NOLINT(xii*)

  template <typename T>
  void InitInplace(const T& value);

  template <typename T>
  void InitShared(const T& value);

  template <typename T>
  void InitTypedObject(const T& value, xiiTraitInt<0>);
  template <typename T>
  void InitTypedObject(const T& value, xiiTraitInt<1>);

  void InitTypedPointer(void* value, const xiiRTTI* pType);

  void Release();
  void CopyFrom(const xiiVariant& other);
  void MoveFrom(xiiVariant&& other);

  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::DirectCast, xiiInt32> = 0>
  const T& Cast() const;
  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::PointerCast, xiiInt32> = 0>
  T Cast() const;
  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::TypedObject, xiiInt32> = 0>
  const T Cast() const;
  template <typename T, typename std::enable_if_t<xiiVariantTypeDeduction<T>::classification == xiiVariantClass::CustomTypeCast, xiiInt32> = 0>
  const T& Cast() const;

  static bool IsNumberStatic(xiiUInt32 type);
  static bool IsFloatingPointStatic(xiiUInt32 type);
  static bool IsStringStatic(xiiUInt32 type);
  static bool IsHashedStringStatic(xiiUInt32 type);
  static bool IsVector2Static(xiiUInt32 type);
  static bool IsVector3Static(xiiUInt32 type);
  static bool IsVector4Static(xiiUInt32 type);
  static bool IsQuatStatic(xiiUInt32 type);
  static bool IsMat3Static(xiiUInt32 type);
  static bool IsMat4Static(xiiUInt32 type);
  static bool IsTransformStatic(xiiUInt32 type);

  // Needed to prevent including xiiRTTI in xiiVariant.h
  static bool          IsDerivedFrom(const xiiRTTI* pType1, const xiiRTTI* pType2);
  static xiiStringView GetTypeName(const xiiRTTI* pType);

  template <typename T>
  T ConvertNumber() const;
};

/// An overload of xiiDynamicCast for dynamic casting a variant to a pointer type.
///
/// If the xiiVariant stores a xiiTypedPointer pointer, this pointer will be dynamically cast to T*.
/// If the xiiVariant stores any other type (or nothing), nullptr is returned.
template <typename T>
XII_ALWAYS_INLINE T xiiDynamicCast(const xiiVariant& variant)
{
  if (variant.IsA<T>())
  {
    return variant.Get<T>();
  }

  return nullptr;
}

// Simple math operator overloads. An invalid variant is returned if the given variants have incompatible types.
XII_FOUNDATION_DLL xiiVariant operator+(const xiiVariant& a, const xiiVariant& b);
XII_FOUNDATION_DLL xiiVariant operator-(const xiiVariant& a, const xiiVariant& b);
XII_FOUNDATION_DLL xiiVariant operator*(const xiiVariant& a, const xiiVariant& b);
XII_FOUNDATION_DLL xiiVariant operator/(const xiiVariant& a, const xiiVariant& b);

namespace xiiMath
{
  /// An overload of xiiMath::Lerp to interpolate variants. A and b must have the same type.
  ///
  /// If the type can't be interpolated like e.g. strings, a is returned for a fFactor less than 0.5, b is returned for a fFactor greater or equal to 0.5.
  XII_FOUNDATION_DLL xiiVariant Lerp(const xiiVariant& a, const xiiVariant& b, double fFactor);
} // namespace xiiMath

#include <Foundation/Types/Implementation/VariantHelper_inl.h>

#include <Foundation/Types/Implementation/Variant_inl.h>
