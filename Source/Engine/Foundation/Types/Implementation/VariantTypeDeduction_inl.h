/// Copyright (c) Theophilus Eriata. All Rights Reserved.

/// \cond

template <>
struct xiiVariantTypeDeduction<bool>
{
  using StorageType = bool;
  using ReturnType  = bool;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Bool;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiInt8>
{
  using StorageType = xiiInt8;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Int8;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt8>
{
  using StorageType = xiiUInt8;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::UInt8;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiInt16>
{
  using StorageType = xiiInt16;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Int16;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt16>
{
  using StorageType = xiiUInt16;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::UInt16;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiInt32>
{
  using StorageType = xiiInt32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Int32;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt32>
{
  using StorageType = xiiUInt32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::UInt32;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiInt64>
{
  using StorageType = xiiInt64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Int64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt64>
{
  using StorageType = xiiUInt64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::UInt64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<float>
{
  using StorageType = float;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Float;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<double>
{
  using StorageType = double;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Double;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiColor>
{
  using StorageType = xiiColor;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Color;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiColorGammaUB>
{
  using StorageType = xiiColorGammaUB;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::ColorGamma;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2>
{
  using StorageType = xiiVec2;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2d>
{
  using StorageType = xiiVec2d;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2d;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3>
{
  using StorageType = xiiVec3;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3d>
{
  using StorageType = xiiVec3d;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3d;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4>
{
  using StorageType = xiiVec4;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4d>
{
  using StorageType = xiiVec4d;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4d;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2I32>
{
  using StorageType = xiiVec2I32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2I;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2I64>
{
  using StorageType = xiiVec2I64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2I64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3I32>
{
  using StorageType = xiiVec3I32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3I;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3I64>
{
  using StorageType = xiiVec3I64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3I64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4I32>
{
  using StorageType = xiiVec4I32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4I;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4I64>
{
  using StorageType = xiiVec4I64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4I64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2U32>
{
  using StorageType = xiiVec2U32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2U;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2U64>
{
  using StorageType = xiiVec2U64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector2U64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3U32>
{
  using StorageType = xiiVec3U32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3U;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3U64>
{
  using StorageType = xiiVec3U64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector3U64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4U32>
{
  using StorageType = xiiVec4U32;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4U;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4U64>
{
  using StorageType = xiiVec4U64;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Vector4U64;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiQuat>
{
  using StorageType = xiiQuat;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Quaternion;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiQuatd>
{
  using StorageType = xiiQuatd;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Quaterniond;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiMat3>
{
  using StorageType = xiiMat3;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Matrix3;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiMat3d>
{
  using StorageType = xiiMat3d;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Matrix3d;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiMat4>
{
  using StorageType = xiiMat4;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Matrix4;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiMat4d>
{
  using StorageType = xiiMat4d;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Matrix4d;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiTransform>
{
  using StorageType = xiiTransform;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Transform;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiTransformd>
{
  using StorageType = xiiTransformd;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Transformd;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiString>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUntrackedString>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiStringView>
{
  using StorageType = xiiStringView;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::StringView;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiHashedString>
{
  using StorageType = xiiHashedString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::HashedString;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiTempHashedString>
{
  using StorageType = xiiTempHashedString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::TempHashedString;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiDataBuffer>
{
  using StorageType = xiiDataBuffer;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::DataBuffer;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<char*>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<const char*>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <size_t N>
struct xiiVariantTypeDeduction<char[N]>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <size_t N>
struct xiiVariantTypeDeduction<const char[N]>
{
  using StorageType = xiiString;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::String;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiTime>
{
  using StorageType = xiiTime;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Time;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiUuid>
{
  using StorageType = xiiUuid;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Uuid;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiAngle>
{
  using StorageType = xiiAngle;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Angle;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiAngled>
{
  using StorageType = xiiAngled;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::Angled;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiVariantArray>
{
  using StorageType = xiiVariantArray;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::VariantArray;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <>
struct xiiVariantTypeDeduction<xiiArrayPtr<xiiVariant>>
{
  using StorageType = xiiVariantArray;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::VariantArray;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};


template <>
struct xiiVariantTypeDeduction<xiiVariantDictionary>
{
  using StorageType = xiiVariantDictionary;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::VariantDictionary;
  static constexpr bool                  forceSharing        = true;
  static constexpr bool                  hasReflectedMembers = false;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

namespace xiiInternal
{
  template <xiiInt32 v>
  struct PointerDeductionHelper
  {
  };

  template <>
  struct PointerDeductionHelper<0>
  {
    using StorageType = void*;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    using StorageType = xiiReflectedClass*;
  };
} // namespace xiiInternal

template <>
struct xiiVariantTypeDeduction<xiiTypedPointer>
{
  using StorageType = xiiTypedPointer;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::TypedPointer;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::DirectCast;
};

template <typename T>
struct xiiVariantTypeDeduction<T*>
{
  using StorageType = xiiTypedPointer;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::TypedPointer;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::PointerCast;
};

template <>
struct xiiVariantTypeDeduction<xiiTypedObject>
{
  using StorageType = xiiTypedObject;

  static constexpr xiiVariantType::Enum  value               = xiiVariantType::TypedObject;
  static constexpr bool                  forceSharing        = false;
  static constexpr bool                  hasReflectedMembers = true;
  static constexpr xiiVariantClass::Enum classification      = xiiVariantClass::TypedObject;
};

/// \endcond
