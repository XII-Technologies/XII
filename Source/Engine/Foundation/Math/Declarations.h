#pragma once

/// \file

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
#  define XII_NAN_ASSERT(obj) (obj)->AssertNotNaN();
#else
#  define XII_NAN_ASSERT(obj)
#endif

/// \brief Simple helper union to store ints and floats to modify their bit patterns.
union xiiIntFloatUnion
{
  constexpr xiiIntFloatUnion(float init) :
    f(init)
  {
  }

  constexpr xiiIntFloatUnion(xiiUInt32 init) :
    i(init)
  {
  }

  xiiUInt32 i;
  float     f;
};

/// \brief Simple helper union to store ints and doubles to modify their bit patterns.
union xiiInt64DoubleUnion
{

  constexpr xiiInt64DoubleUnion(double init) :
    f(init)
  {
  }
  constexpr xiiInt64DoubleUnion(xiiUInt64 init) :
    i(init)
  {
  }

  xiiUInt64 i;
  double    f;
};

/// \brief Enum to describe which memory layout is used to store a matrix in a float array.
///
/// All xiiMatX classes use column-major format internally. That means they contain one array
/// of, e.g. 16 elements, and the first elements represent the first column, then the second column, etc.
/// So the data is stored column by column and is thus column-major.
/// Some other libraries, such as OpenGL or DirectX require data represented either in column-major
/// or row-major format. xiiMatrixLayout allows to retrieve the data from an xiiMatX class in the proper format,
/// and it also allows to pass matrix data as an array back in the xiiMatX class, and have it converted properly.
/// That means, if you need to pass the content of an xiiMatX to a function that requires the data in row-major
/// format, you specify that you want to convert the matrix to xiiMatrixLayout::RowMajor format and you will get
/// the data properly transposed. If a function requires data in column-major format, you specify
/// xiiMatrixLayout::ColumnMajor and you get it in column-major format (which is simply a memcpy).
struct xiiMatrixLayout
{
  enum Enum
  {
    RowMajor,   ///< The matrix is stored in row-major format.
    ColumnMajor ///< The matrix is stored in column-major format.
  };
};

/// \brief Describes for which depth range a projection matrix is constructed.
///
/// Different Rendering APIs use different depth ranges.
/// E.g. OpenGL uses -1 for the near plane and +1 for the far plane.
/// DirectX uses 0 for the near plane and 1 for the far plane.
struct xiiClipSpaceDepthRange
{
  enum Enum
  {
    MinusOneToOne, ///< Near plane at -1, far plane at +1
    ZeroToOne,     ///< Near plane at 0, far plane at 1
  };

  /// \brief Holds the default value for the projection depth range on each platform.
  /// This can be overridden by renderers to ensure the proper range is used when they become active.
  /// On Windows/D3D this is initialized with 'ZeroToOne' by default on all other platforms/OpenGL it is initialized with 'MinusOneToOne' by default.
  XII_FOUNDATION_DLL static Enum Default;
};

/// \brief Specifies whether a projection matrix should flip the result along the Y axis or not.
///
/// Mostly needed to compensate for differing Y texture coordinate conventions. Ie. on some platforms
/// the Y texture coordinate origin is at the lower left and on others on the upper left. To prevent having
/// to modify content to compensate, instead textures are simply flipped along Y on texture load.
/// The same has to be done for all render targets, ie. content has to be rendered upside-down.
///
/// Use xiiClipSpaceYMode::RenderToTextureDefault when rendering to a texture, to always get the correct
/// projection matrix.
struct xiiClipSpaceYMode
{
  enum Enum
  {
    Regular, ///< Creates a regular projection matrix
    Flipped, ///< Creates a projection matrix that flips the image on its head. On platforms with different Y texture coordinate
             ///< conventions, this can be used to compensate, by rendering images flipped to render targets.
  };

  /// \brief Holds the platform default value for the clip space Y mode when rendering to a texture.
  /// This can be overridden by renderers to ensure the proper mode is used when they become active.
  /// On Windows/D3D this is initialized with 'Regular' by default on all other platforms/OpenGL it is initialized with 'Flipped' by default.
  XII_FOUNDATION_DLL static Enum RenderToTextureDefault;
};

/// \brief For selecting a left-handed or right-handed convention
struct xiiHandedness
{
  enum Enum
  {
    LeftHanded,
    RightHanded,
  };

  /// \brief Holds the default handedness value to use. XII uses 'LeftHanded' by default.
  XII_FOUNDATION_DLL static Enum Default /*= xiiHandedness::LeftHanded*/;
};

/// \brief Enumeration that describes which underlying graphics device implementation to use.
struct xiiGraphicsDevice
{
  enum Enum
  {
    Undefined = 0,

    D3D11,
    D3D12,
    Metal,
    OpenGL,
    Vulkan,
  };

  /// \brief Holds the default graphics device to use.
  XII_FOUNDATION_DLL static Enum Default /*= xiiGraphicsDevice::Undefined*/;
};

// forward declarations
template <typename Type>
class xiiVec2Template;

using xiiVec2     = xiiVec2Template<float>;
using xiiVec2d    = xiiVec2Template<double>;
using xiiVec2Real = xiiVec2Template<xiiReal>;
using xiiVec2I32  = xiiVec2Template<xiiInt32>;
using xiiVec2I64  = xiiVec2Template<xiiInt64>;
using xiiVec2U32  = xiiVec2Template<xiiUInt32>;
using xiiVec2U64  = xiiVec2Template<xiiUInt64>;

template <typename Type>
class xiiVec3Template;

using xiiVec3     = xiiVec3Template<float>;
using xiiVec3d    = xiiVec3Template<double>;
using xiiVec3Real = xiiVec3Template<xiiReal>;
using xiiVec3I32  = xiiVec3Template<xiiInt32>;
using xiiVec3I64  = xiiVec3Template<xiiInt64>;
using xiiVec3U32  = xiiVec3Template<xiiUInt32>;
using xiiVec3U64  = xiiVec3Template<xiiUInt64>;

template <typename Type>
class xiiVec4Template;

using xiiVec4     = xiiVec4Template<float>;
using xiiVec4d    = xiiVec4Template<double>;
using xiiVec4Real = xiiVec4Template<xiiReal>;
using xiiVec4I64  = xiiVec4Template<xiiInt64>;
using xiiVec4I32  = xiiVec4Template<xiiInt32>;
using xiiVec4I16  = xiiVec4Template<xiiInt16>;
using xiiVec4I8   = xiiVec4Template<xiiInt8>;
using xiiVec4U64  = xiiVec4Template<xiiUInt64>;
using xiiVec4U32  = xiiVec4Template<xiiUInt32>;
using xiiVec4U16  = xiiVec4Template<xiiUInt16>;
using xiiVec4U8   = xiiVec4Template<xiiUInt8>;

template <typename Type>
class xiiMat3Template;

using xiiMat3     = xiiMat3Template<float>;
using xiiMat3d    = xiiMat3Template<double>;
using xiiMat3Real = xiiMat3Template<xiiReal>;

template <typename Type>
class xiiMat4Template;

using xiiMat4     = xiiMat4Template<float>;
using xiiMat4d    = xiiMat4Template<double>;
using xiiMat4Real = xiiMat4Template<xiiReal>;

template <typename Type>
struct xiiPlaneTemplate;

using xiiPlane     = xiiPlaneTemplate<float>;
using xiiPlaned    = xiiPlaneTemplate<double>;
using xiiPlaneReal = xiiPlaneTemplate<xiiReal>;

template <typename Type>
class xiiQuatTemplate;

using xiiQuat     = xiiQuatTemplate<float>;
using xiiQuatd    = xiiQuatTemplate<double>;
using xiiQuatReal = xiiQuatTemplate<xiiReal>;

template <typename Type>
class xiiBoundingBoxTemplate;

using xiiBoundingBox     = xiiBoundingBoxTemplate<float>;
using xiiBoundingBoxd    = xiiBoundingBoxTemplate<double>;
using xiiBoundingBoxReal = xiiBoundingBoxTemplate<xiiReal>;
using xiiBoundingBoxu32  = xiiBoundingBoxTemplate<xiiUInt32>;
using xiiBoundingBoxu64  = xiiBoundingBoxTemplate<xiiUInt64>;

template <typename Type>
class xiiBoundingBoxSphereTemplate;

using xiiBoundingBoxSphere     = xiiBoundingBoxSphereTemplate<float>;
using xiiBoundingBoxSphered    = xiiBoundingBoxSphereTemplate<double>;
using xiiBoundingBoxSphereReal = xiiBoundingBoxSphereTemplate<xiiReal>;

template <typename Type>
class xiiBoundingSphereTemplate;

using xiiBoundingSphere     = xiiBoundingSphereTemplate<float>;
using xiiBoundingSphered    = xiiBoundingSphereTemplate<double>;
using xiiBoundingSphereReal = xiiBoundingSphereTemplate<xiiReal>;

template <xiiUInt8 DecimalBits>
class xiiFixedPoint;

template <typename Type>
class xiiAngleTemplate;

using xiiAngle     = xiiAngleTemplate<float>;
using xiiAngled    = xiiAngleTemplate<double>;
using xiiAngleReal = xiiAngleTemplate<xiiReal>;

template <typename Type>
class xiiTransformTemplate;

using xiiTransform     = xiiTransformTemplate<float>;
using xiiTransformd    = xiiTransformTemplate<double>;
using xiiTransformReal = xiiTransformTemplate<xiiReal>;

class xiiColor;
class xiiColorLinearUB;
class xiiColorGammaUB;

class xiiRandom;


/// \brief An enum that allows to select on of the six main axis (positive / negative)
struct XII_FOUNDATION_DLL xiiBasisAxis
{
  using StorageType = xiiInt8;

  /// \brief An enum that allows to select on of the six main axis (positive / negative)
  enum Enum : xiiInt8
  {
    PositiveX,
    PositiveY,
    PositiveZ,
    NegativeX,
    NegativeY,
    NegativeZ,

    Default = PositiveX
  };

  /// \brief Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static xiiVec3 GetBasisVectorFloat(xiiBasisAxis::Enum basisAxis);

  /// \brief Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static xiiVec3d GetBasisVectorDouble(xiiBasisAxis::Enum basisAxis);

  /// \brief Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static xiiVec3Real GetBasisVectorReal(xiiBasisAxis::Enum basisAxis);

  /// \brief Computes a matrix representing the transformation. 'Forward' represents the X axis, 'Right' the Y axis and 'Up' the Z axis.
  static xiiMat3 CalculateTransformationMatrix(xiiBasisAxis::Enum forwardDir, xiiBasisAxis::Enum rightDir, xiiBasisAxis::Enum upDir, float fUniformScale = 1.0f, float fScaleX = 1.0f, float fScaleY = 1.0f, float fScaleZ = 1.0f);

  /// \brief Computes a matrix representing the transformation. 'Forward' represents the X axis, 'Right' the Y axis and 'Up' the Z axis.
  static xiiMat3d CalculateTransformationMatrix(xiiBasisAxis::Enum forwardDir, xiiBasisAxis::Enum rightDir, xiiBasisAxis::Enum upDir, double fUniformScale = 1.0, double fScaleX = 1.0, double fScaleY = 1.0, double fScaleZ = 1.0);

  /// \brief Returns a quaternion that rotates from 'identity' to 'axis'
  static xiiQuat GetBasisRotation(xiiBasisAxis::Enum identity, xiiBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'identity' to 'axis'
  static xiiQuatd GetBasisRotationDouble(xiiBasisAxis::Enum identity, xiiBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'identity' to 'axis'
  static xiiQuatReal GetBasisRotationReal(xiiBasisAxis::Enum identity, xiiBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static xiiQuat GetBasisRotation_PosX(xiiBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static xiiQuatd GetBasisRotationDouble_PosX(xiiBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static xiiQuatReal GetBasisRotationReal_PosX(xiiBasisAxis::Enum axis);

  /// \brief Returns the axis that is orthogonal to axis1 and axis2. If 'flip' is set, it returns the negated axis.
  ///
  /// If axis1 and axis2 are not orthogonal to each other, the value of axis1 is returned as the result.
  static xiiBasisAxis::Enum GetOrthogonalAxis(xiiBasisAxis::Enum axis1, xiiBasisAxis::Enum axis2, bool flip);
};

/// \brief An enum that represents the operator of a comparison
struct XII_FOUNDATION_DLL xiiComparisonOperator
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal
  };

  static bool Compare(xiiComparisonOperator::Enum cmp, double f1, double f2);
};
