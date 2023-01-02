#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Types.h>

class xiiReflectedClass;
class xiiVariant;
struct xiiTime;
class xiiUuid;
struct xiiStringView;
struct xiiTypedObject;
struct xiiTypedPointer;

using xiiDataBuffer        = xiiDynamicArray<xiiUInt8>;
using xiiVariantArray      = xiiDynamicArray<xiiVariant>;
using xiiVariantDictionary = xiiHashTable<xiiString, xiiVariant>;

/// \brief This enum describes the type of data that is currently stored inside the variant.
struct xiiVariantType
{
  using StorageType = xiiUInt8;
  /// \brief This enum describes the type of data that is currently stored inside the variant.
  /// Note that changes to this enum require an increase of the reflection version and either
  /// patches to the serializer or a re-export of binary data that contains xiiVariants.
  enum Enum : xiiUInt8
  {
    Invalid = 0, ///< The variant stores no (valid) data at the moment.

    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***
    FirstStandardType = 1,
    Bool,        ///< The variant stores a bool.
    Int8,        ///< The variant stores an xiiInt8.
    UInt8,       ///< The variant stores an xiiUInt8.
    Int16,       ///< The variant stores an xiiInt16.
    UInt16,      ///< The variant stores an xiiUInt16.
    Int32,       ///< The variant stores an xiiInt32.
    UInt32,      ///< The variant stores an xiiUInt32.
    Int64,       ///< The variant stores an xiiInt64.
    UInt64,      ///< The variant stores an xiiUInt64.
    Float,       ///< The variant stores a float.
    Double,      ///< The variant stores a double.
    Color,       ///< The variant stores an xiiColor.
    Vector2,     ///< The variant stores an xiiVec2.
    Vector2d,    ///< The variant stores an xiiVec2d.
    Vector3,     ///< The variant stores an xiiVec3.
    Vector3d,    ///< The variant stores an xiiVec3d.
    Vector4,     ///< The variant stores an xiiVec4.
    Vector4d,    ///< The variant stores an xiiVec4d.
    Vector2I,    ///< The variant stores an xiiVec2I32.
    Vector2I64,  ///< The variant stores an xiiVec2I64.
    Vector3I,    ///< The variant stores an xiiVec3I32.
    Vector3I64,  ///< The variant stores an xiiVec3I64.
    Vector4I,    ///< The variant stores an xiiVec4I32.
    Vector4I64,  ///< The variant stores an xiiVec4I64.
    Vector2U,    ///< The variant stores an xiiVec2U32.
    Vector2U64,  ///< The variant stores an xiiVec2U64.
    Vector3U,    ///< The variant stores an xiiVec3U32.
    Vector3U64,  ///< The variant stores an xiiVec3U64.
    Vector4U,    ///< The variant stores an xiiVec4U32.
    Vector4U64,  ///< The variant stores an xiiVec4U64.
    Quaternion,  ///< The variant stores an xiiQuat.
    Quaterniond, ///< The variant stores an xiiQuatd.
    Matrix3,     ///< The variant stores an xiiMat3. A heap allocation is required to store this data type.
    Matrix3d,    ///< The variant stores an xiiMat3d. A heap allocation is required to store this data type.
    Matrix4,     ///< The variant stores an xiiMat4. A heap allocation is required to store this data type.
    Matrix4d,    ///< The variant stores an xiiMat4d. A heap allocation is required to store this data type.
    Transform,   ///< The variant stores an xiiTransform. A heap allocation is required to store this data type.
    Transformd,  ///< The variant stores an xiiTransformd. A heap allocation is required to store this data type.
    String,      ///< The variant stores a string. A heap allocation is required to store this data type.
    StringView,  ///< The variant stores an xiiStringView.
    DataBuffer,  ///< The variant stores an xiiDataBuffer, a typedef to DynamicArray<xiiUInt8>. A heap allocation is required to store this data type.
    Time,        ///< The variant stores an xiiTime value.
    Uuid,        ///< The variant stores an xiiUuid value.
    Angle,       ///< The variant stores an xiiAngle value.
    ColorGamma,  ///< The variant stores an xiiColorGammaUB value.
    LastStandardType,
    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***

    FirstExtendedType = 64,
    VariantArray,      ///< The variant stores an array of xiiVariant's. A heap allocation is required to store this data type.
    VariantDictionary, ///< The variant stores a dictionary (hashmap) of xiiVariant's. A heap allocation is required to store this type.
    TypedPointer,      ///< The variant stores an xiiTypedPointer value. Reflected type and data queries will match the pointed to object.
    TypedObject,       ///< The variant stores an xiiTypedObject value. Reflected type and data queries will match the object. A heap allocation is required to store this type if it is larger than 16 bytes or not POD.
    LastExtendedType,  ///< Number of values for xiiVariant::Type.

    MAX_ENUM_VALUE = LastExtendedType,
    Default        = Invalid ///< Default value used by xiiEnum.
  };
};

XII_DEFINE_AS_POD_TYPE(xiiVariantType::Enum);

struct xiiVariantClass
{
  enum Enum
  {
    Invalid,
    DirectCast,     ///< A standard type
    PointerCast,    ///< Any cast to T*
    TypedObject,    ///< xiiTypedObject cast. Needed because at no point does and xiiVariant ever store a xiiTypedObject so it can't be returned as a const reference.
    CustomTypeCast, ///< Custom object types
  };
};

/// \brief A helper struct to convert the C++ type, which is passed as the template argument, into one of the xiiVariant::Type enum values.
template <typename T>
struct xiiVariantTypeDeduction
{
  enum
  {
    value               = xiiVariantType::Invalid,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::Invalid
  };

  using StorageType = T;
};

/// \brief Declares a custom variant type, allowing it to be stored by value inside an xiiVariant.
///
/// Needs to be called from the same header that defines the type.
/// \sa XII_DEFINE_CUSTOM_VARIANT_TYPE
#define XII_DECLARE_CUSTOM_VARIANT_TYPE(TYPE)               \
  template <>                                               \
  struct xiiVariantTypeDeduction<TYPE>                      \
  {                                                         \
    enum                                                    \
    {                                                       \
      value               = xiiVariantType::TypedObject,    \
      forceSharing        = false,                          \
      hasReflectedMembers = true,                           \
      classification      = xiiVariantClass::CustomTypeCast \
    };                                                      \
    using StorageType = TYPE;                               \
  };

#include <Foundation/Types/Implementation/VariantTypeDeduction_inl.h>
