/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Types.h>

class xiiReflectedClass;
class xiiVariant;
struct xiiTime;
class xiiUuid;
class xiiStringView;
struct xiiTypedObject;
struct xiiTypedPointer;

using xiiDataBuffer        = xiiDynamicArray<xiiUInt8>;
using xiiVariantArray      = xiiDynamicArray<xiiVariant>;
using xiiVariantDictionary = xiiHashTable<xiiString, xiiVariant>;

/// This enum describes the type of data that is currently stored inside the variant.
struct xiiVariantType
{
  using StorageType = xiiUInt8;
  /// This enum describes the type of data that is currently stored inside the variant.
  /// Note that changes to this enum require an increase of the reflection version and either
  /// patches to the serializer or a re-export of binary data that contains xiiVariants.
  enum Enum : StorageType
  {
    Invalid = 0U, ///< The variant stores no (valid) data at the moment.

    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***
    FirstStandardType = 1U,
    Bool,             ///< The variant stores a bool.
    Int8,             ///< The variant stores a xiiInt8.
    Int16,            ///< The variant stores a xiiInt16.
    Int32,            ///< The variant stores a xiiInt32.
    Int64,            ///< The variant stores a xiiInt64.
    UInt8,            ///< The variant stores a xiiUInt8.
    UInt16,           ///< The variant stores a xiiUInt16.
    UInt32,           ///< The variant stores a xiiUInt32.
    UInt64,           ///< The variant stores a xiiUInt64.
    Float,            ///< The variant stores a float.
    Double,           ///< The variant stores a double.
    Color,            ///< The variant stores a xiiColor.
    Vector2I,         ///< The variant stores a xiiVec2I32.
    Vector3I,         ///< The variant stores a xiiVec3I32.
    Vector4I,         ///< The variant stores a xiiVec4I32.
    Vector2I64,       ///< The variant stores a xiiVec2I64.
    Vector3I64,       ///< The variant stores a xiiVec3I64. A heap allocation is required to store this data type.
    Vector4I64,       ///< The variant stores a xiiVec4I64. A heap allocation is required to store this data type.
    Vector2U,         ///< The variant stores a xiiVec2U32.
    Vector3U,         ///< The variant stores a xiiVec3U32.
    Vector4U,         ///< The variant stores a xiiVec4U32.
    Vector2U64,       ///< The variant stores a xiiVec2U64.
    Vector3U64,       ///< The variant stores a xiiVec3U64. A heap allocation is required to store this data type.
    Vector4U64,       ///< The variant stores a xiiVec4U64. A heap allocation is required to store this data type.
    Vector2,          ///< The variant stores a xiiVec2.
    Vector3,          ///< The variant stores a xiiVec3.
    Vector4,          ///< The variant stores a xiiVec4.
    Vector2d,         ///< The variant stores a xiiVec2d.
    Vector3d,         ///< The variant stores a xiiVec3d. A heap allocation is required to store this data type.
    Vector4d,         ///< The variant stores a xiiVec4d. A heap allocation is required to store this data type.
    Quaternion,       ///< The variant stores a xiiQuat.
    Quaterniond,      ///< The variant stores a xiiQuatd. A heap allocation is required to store this data type.
    Matrix3,          ///< The variant stores a xiiMat3. A heap allocation is required to store this data type.
    Matrix3d,         ///< The variant stores a xiiMat3d. A heap allocation is required to store this data type.
    Matrix4,          ///< The variant stores a xiiMat4. A heap allocation is required to store this data type.
    Matrix4d,         ///< The variant stores a xiiMat4d. A heap allocation is required to store this data type.
    Transform,        ///< The variant stores a xiiTransform. A heap allocation is required to store this data type.
    Transformd,       ///< The variant stores a xiiTransformd. A heap allocation is required to store this data type.
    Angle,            ///< The variant stores a xiiAngle value.
    Angled,           ///< The variant stores a xiiAngled value.
    String,           ///< The variant stores a string. A heap allocation is required to store this data type.
    StringView,       ///< The variant stores a xiiStringView.
    HashedString,     ///< The variant stores a xiiHashedString value.
    TempHashedString, ///< The variant stores a xiiTempHashedString value.
    DataBuffer,       ///< The variant stores a xiiDataBuffer, an alias to DynamicArray<xiiUInt8>. A heap allocation is required to store this data type.
    Time,             ///< The variant stores a xiiTime value.
    Uuid,             ///< The variant stores a xiiUuid value.
    ColorGamma,       ///< The variant stores a xiiColorGammaUB value.
    LastStandardType,
    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***

    FirstExtendedType = 64U,
    VariantArray,      ///< The variant stores an array of xiiVariant's. A heap allocation is required to store this data type.
    VariantDictionary, ///< The variant stores a dictionary (hashmap) of xiiVariant's. A heap allocation is required to store this type.
    TypedPointer,      ///< The variant stores a xiiTypedPointer value. Reflected type and data queries will match the pointed to object.
    TypedObject,       ///< The variant stores a xiiTypedObject value. Reflected type and data queries will match the object. A heap allocation is required to store this type if it is larger than 16 bytes or not POD.
    LastExtendedType,  ///< Number of values for xiiVariant::Type.

    MAX_ENUM_VALUE = LastExtendedType,
    Default        = Invalid ///< Default value used by xiiEnum.
  };
};

XII_DEFINE_AS_POD_TYPE(xiiVariantType::Enum);

struct xiiVariantClass
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Invalid = 0U,
    DirectCast,     ///< A standard type.
    PointerCast,    ///< Any cast to T*.
    TypedObject,    ///< xiiTypedObject cast. Needed because at no point does and xiiVariant ever store a xiiTypedObject so it can't be returned as a const reference.
    CustomTypeCast, ///< Custom object types.
  };
};

/// A helper struct to convert the C++ type, which is passed as the template argument, into one of the xiiVariant::Type enum values.
template <typename T>
struct xiiVariantTypeDeduction
{
  using StorageType = T;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Invalid;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::Invalid;
};

/// Declares a custom variant type, allowing it to be stored by value inside a xiiVariant.
///
/// Needs to be called from the same header that defines the type.
/// \sa XII_DEFINE_CUSTOM_VARIANT_TYPE
#define XII_DECLARE_CUSTOM_VARIANT_TYPE(TYPE)                                                     \
  template <>                                                                                     \
  struct xiiVariantTypeDeduction<TYPE>                                                            \
  {                                                                                               \
    using StorageType                                          = TYPE;                            \
    static constexpr xiiVariantType::Enum  value               = xiiVariantType::TypedObject;     \
    static constexpr bool                  forceSharing        = false;                           \
    static constexpr bool                  hasReflectedMembers = true;                            \
    static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::CustomTypeCast; \
  };

#include <Foundation/Types/Implementation/VariantTypeDeduction_inl.h>
