


/// \cond

template <>
struct xiiVariantTypeDeduction<bool>
{
  enum
  {
    value               = xiiVariantType::Bool,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = bool;
  using ReturnType  = bool;
};

template <>
struct xiiVariantTypeDeduction<xiiInt8>
{
  enum
  {
    value               = xiiVariantType::Int8,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiInt8;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt8>
{
  enum
  {
    value               = xiiVariantType::UInt8,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiUInt8;
};

template <>
struct xiiVariantTypeDeduction<xiiInt16>
{
  enum
  {
    value               = xiiVariantType::Int16,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiInt16;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt16>
{
  enum
  {
    value               = xiiVariantType::UInt16,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiUInt16;
};

template <>
struct xiiVariantTypeDeduction<xiiInt32>
{
  enum
  {
    value               = xiiVariantType::Int32,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiInt32;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt32>
{
  enum
  {
    value               = xiiVariantType::UInt32,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiUInt32;
};

template <>
struct xiiVariantTypeDeduction<xiiInt64>
{
  enum
  {
    value               = xiiVariantType::Int64,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiInt64;
};

template <>
struct xiiVariantTypeDeduction<xiiUInt64>
{
  enum
  {
    value               = xiiVariantType::UInt64,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiUInt64;
};

template <>
struct xiiVariantTypeDeduction<float>
{
  enum
  {
    value               = xiiVariantType::Float,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = float;
};

template <>
struct xiiVariantTypeDeduction<double>
{
  enum
  {
    value               = xiiVariantType::Double,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = double;
};

template <>
struct xiiVariantTypeDeduction<xiiColor>
{
  enum
  {
    value               = xiiVariantType::Color,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiColor;
};

template <>
struct xiiVariantTypeDeduction<xiiColorGammaUB>
{
  enum
  {
    value               = xiiVariantType::ColorGamma,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiColorGammaUB;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2>
{
  enum
  {
    value               = xiiVariantType::Vector2,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec2;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3>
{
  enum
  {
    value               = xiiVariantType::Vector3,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec3;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4>
{
  enum
  {
    value               = xiiVariantType::Vector4,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec4;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2I32>
{
  enum
  {
    value               = xiiVariantType::Vector2I,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec2I32;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3I32>
{
  enum
  {
    value               = xiiVariantType::Vector3I,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec3I32;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4I32>
{
  enum
  {
    value               = xiiVariantType::Vector4I,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec4I32;
};

template <>
struct xiiVariantTypeDeduction<xiiVec2U32>
{
  enum
  {
    value               = xiiVariantType::Vector2U,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec2U32;
};

template <>
struct xiiVariantTypeDeduction<xiiVec3U32>
{
  enum
  {
    value               = xiiVariantType::Vector3U,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec3U32;
};

template <>
struct xiiVariantTypeDeduction<xiiVec4U32>
{
  enum
  {
    value               = xiiVariantType::Vector4U,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVec4U32;
};

template <>
struct xiiVariantTypeDeduction<xiiQuat>
{
  enum
  {
    value               = xiiVariantType::Quaternion,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiQuat;
};

template <>
struct xiiVariantTypeDeduction<xiiMat3>
{
  enum
  {
    value               = xiiVariantType::Matrix3,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiMat3;
};

template <>
struct xiiVariantTypeDeduction<xiiMat4>
{
  enum
  {
    value               = xiiVariantType::Matrix4,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiMat4;
};

template <>
struct xiiVariantTypeDeduction<xiiTransform>
{
  enum
  {
    value               = xiiVariantType::Transform,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiTransform;
};

template <>
struct xiiVariantTypeDeduction<xiiString>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <>
struct xiiVariantTypeDeduction<xiiUntrackedString>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <>
struct xiiVariantTypeDeduction<xiiStringView>
{
  enum
  {
    value               = xiiVariantType::StringView,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiStringView;
};

template <>
struct xiiVariantTypeDeduction<xiiDataBuffer>
{
  enum
  {
    value               = xiiVariantType::DataBuffer,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiDataBuffer;
};

template <>
struct xiiVariantTypeDeduction<char*>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <>
struct xiiVariantTypeDeduction<const char*>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <size_t N>
struct xiiVariantTypeDeduction<char[N]>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <size_t N>
struct xiiVariantTypeDeduction<const char[N]>
{
  enum
  {
    value               = xiiVariantType::String,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiString;
};

template <>
struct xiiVariantTypeDeduction<xiiTime>
{
  enum
  {
    value               = xiiVariantType::Time,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiTime;
};

template <>
struct xiiVariantTypeDeduction<xiiUuid>
{
  enum
  {
    value               = xiiVariantType::Uuid,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiUuid;
};

template <>
struct xiiVariantTypeDeduction<xiiAngle>
{
  enum
  {
    value               = xiiVariantType::Angle,
    forceSharing        = false,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiAngle;
};

template <>
struct xiiVariantTypeDeduction<xiiVariantArray>
{
  enum
  {
    value               = xiiVariantType::VariantArray,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVariantArray;
};

template <>
struct xiiVariantTypeDeduction<xiiArrayPtr<xiiVariant>>
{
  enum
  {
    value               = xiiVariantType::VariantArray,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVariantArray;
};


template <>
struct xiiVariantTypeDeduction<xiiVariantDictionary>
{
  enum
  {
    value               = xiiVariantType::VariantDictionary,
    forceSharing        = true,
    hasReflectedMembers = false,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiVariantDictionary;
};

namespace xiiInternal
{
  template <int v>
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
  enum
  {
    value               = xiiVariantType::TypedPointer,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::DirectCast
  };

  using StorageType = xiiTypedPointer;
};

template <typename T>
struct xiiVariantTypeDeduction<T*>
{
  enum
  {
    value               = xiiVariantType::TypedPointer,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::PointerCast
  };

  using StorageType = xiiTypedPointer;
};

template <>
struct xiiVariantTypeDeduction<xiiTypedObject>
{
  enum
  {
    value               = xiiVariantType::TypedObject,
    forceSharing        = false,
    hasReflectedMembers = true,
    classification      = xiiVariantClass::TypedObject
  };

  using StorageType = xiiTypedObject;
};

/// \endcond
