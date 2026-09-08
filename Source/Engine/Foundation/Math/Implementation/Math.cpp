/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Easing.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>

// Default are D3D convention before a renderer is initialized.
xiiClipSpaceDepthRange::Enum xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
xiiClipSpaceYMode::Enum      xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

xiiHandedness::Enum xiiHandedness::Default = xiiHandedness::LeftHanded;

bool xiiMath::IsPowerOf(xiiInt32 value, xiiInt32 iBase)
{
  if (value == 1)
    return true;

  while (value > iBase)
  {
    if (value % iBase == 0)
      value /= iBase;
    else
      return false;
  }

  return (value == iBase);
}

xiiUInt32 xiiMath::PowerOfTwo_Floor(xiiUInt32 uiNpot)
{
  return static_cast<xiiUInt32>(PowerOfTwo_Floor(static_cast<xiiUInt64>(uiNpot)));
}

xiiUInt64 xiiMath::PowerOfTwo_Floor(xiiUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (xiiUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
      return (uiNpot << i);
  }

  return (1);
}

xiiUInt32 xiiMath::PowerOfTwo_Ceil(xiiUInt32 uiNpot)
{
  return static_cast<xiiUInt32>(PowerOfTwo_Ceil(static_cast<xiiUInt64>(uiNpot)));
}

xiiUInt64 xiiMath::PowerOfTwo_Ceil(xiiUInt64 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (xiiUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return uiNpot << (i + 1u);
    }
  }

  return (1u);
}

xiiUInt32 xiiMath::GreatestCommonDivisor(xiiUInt32 a, xiiUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return b;
  }
  if (b == 0)
  {
    return a;
  }

  xiiUInt32 uiShift = FirstBitLow(a | b);
  a >>= FirstBitLow(a);
  do
  {
    b >>= FirstBitLow(b);
    if (a > b)
    {
      Swap(a, b);
    }
    b = b - a;
  } while (b != 0);
  return a << uiShift;
}

xiiResult xiiMath::TryMultiply32(xiiUInt32& out_uiResult, xiiUInt32 a, xiiUInt32 b, xiiUInt32 c, xiiUInt32 d)
{
  xiiUInt64 uiResult = static_cast<xiiUInt64>(a) * static_cast<xiiUInt64>(b);

  if (uiResult > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  uiResult *= static_cast<xiiUInt64>(c);

  if (uiResult > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  uiResult *= static_cast<xiiUInt64>(d);

  if (uiResult > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  out_uiResult = static_cast<xiiUInt32>(uiResult & 0xFFFFFFFFllu);
  return XII_SUCCESS;
}

xiiUInt32 xiiMath::SafeMultiply32(xiiUInt32 a, xiiUInt32 b, xiiUInt32 c, xiiUInt32 d)
{
  xiiUInt32 uiResult = 0;
  if (TryMultiply32(uiResult, a, b, c, d).Succeeded())
  {
    return uiResult;
  }

  XII_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds UInt32 range.", a, b, c, d);
  std::terminate();
}

xiiResult xiiMath::TryMultiply64(xiiUInt64& out_uiResult, xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, xiiUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_uiResult = 0;
    return XII_SUCCESS;
  }

#if XII_ENABLED(XII_PLATFORM_ARCH_X86) && XII_ENABLED(XII_PLATFORM_64BIT) && XII_ENABLED(XII_COMPILER_MSVC)

  xiiUInt64 uiHighBits = 0;

  const xiiUInt64 ab = _umul128(a, b, &uiHighBits);
  if (uiHighBits != 0)
  {
    return XII_FAILURE;
  }

  const xiiUInt64 abc = _umul128(ab, c, &uiHighBits);
  if (uiHighBits != 0)
  {
    return XII_FAILURE;
  }

  const xiiUInt64 abcd = _umul128(abc, d, &uiHighBits);
  if (uiHighBits != 0)
  {
    return XII_FAILURE;
  }

#else
  const xiiUInt64 ab   = a * b;
  const xiiUInt64 abc  = ab * c;
  const xiiUInt64 abcd = abc * d;

  if (a > 1 && b > 1 && (ab / a != b))
  {
    return XII_FAILURE;
  }

  if (c > 1 && (abc / c != ab))
  {
    return XII_FAILURE;
  }

  if (d > 1 && (abcd / d != abc))
  {
    return XII_FAILURE;
  }

#endif

  out_uiResult = abcd;
  return XII_SUCCESS;
}

xiiUInt64 xiiMath::SafeMultiply64(xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, xiiUInt64 d)
{
  xiiUInt64 uiResult = 0;
  if (TryMultiply64(uiResult, a, b, c, d).Succeeded())
  {
    return uiResult;
  }

  XII_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds xiiUInt64 range.", a, b, c, d);
  std::terminate();
}

#if XII_ENABLED(XII_PLATFORM_32BIT)
size_t xiiMath::SafeConvertToSizeT(xiiUInt64 uiValue)
{
  size_t uiResult = 0;
  if (TryConvertToSizeT(uiResult, uiValue).Succeeded())
  {
    return uiResult;
  }

  XII_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
}
#endif

float xiiMath::ReplaceNaN(float fValue, float fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (xiiMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

double xiiMath::ReplaceNaN(double fValue, double fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (xiiMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

xiiVec3 xiiBasisAxis::GetBasisVector(Enum basisAxis)
{
  switch (basisAxis)
  {
    case xiiBasisAxis::PositiveX:
      return xiiVec3(1.0f, 0.0f, 0.0f);

    case xiiBasisAxis::NegativeX:
      return xiiVec3(-1.0f, 0.0f, 0.0f);

    case xiiBasisAxis::PositiveY:
      return xiiVec3(0.0f, 1.0f, 0.0f);

    case xiiBasisAxis::NegativeY:
      return xiiVec3(0.0f, -1.0f, 0.0f);

    case xiiBasisAxis::PositiveZ:
      return xiiVec3(0.0f, 0.0f, 1.0f);

    case xiiBasisAxis::NegativeZ:
      return xiiVec3(0.0f, 0.0f, -1.0f);

    default:
      XII_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
      return xiiVec3::MakeZero();
  }
}

xiiVec3d xiiBasisAxis::GetBasisVectorDouble(xiiBasisAxis::Enum basisAxis)
{
  switch (basisAxis)
  {
    case xiiBasisAxis::PositiveX:
      return xiiVec3d(1.0, 0.0, 0.0);

    case xiiBasisAxis::NegativeX:
      return xiiVec3d(-1.0, 0.0, 0.0);

    case xiiBasisAxis::PositiveY:
      return xiiVec3d(0.0, 1.0, 0.0);

    case xiiBasisAxis::NegativeY:
      return xiiVec3d(0.0, -1.0, 0.0);

    case xiiBasisAxis::PositiveZ:
      return xiiVec3d(0.0, 0.0, 1.0);

    case xiiBasisAxis::NegativeZ:
      return xiiVec3d(0.0, 0.0, -1.0);

    default:
      XII_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
      return xiiVec3d::MakeZero();
  }
}

xiiMat3 xiiBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum dir, float fUniformScale /*= 1.0f*/, float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  xiiMat3 mResult;
  mResult.SetRow(0, xiiBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, xiiBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, xiiBasisAxis::GetBasisVector(dir) * fUniformScale * fScaleZ);

  return mResult;
}

xiiMat3d xiiBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum dir, double fUniformScale /*= 1.0f*/, double fScaleX /*= 1.0*/, double fScaleY /*= 1.0*/, double fScaleZ /*= 1.0*/)
{
  xiiMat3d mResult;
  mResult.SetRow(0, xiiBasisAxis::GetBasisVectorDouble(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, xiiBasisAxis::GetBasisVectorDouble(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, xiiBasisAxis::GetBasisVectorDouble(dir) * fUniformScale * fScaleZ);

  return mResult;
}

xiiQuat xiiBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  return xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), GetBasisVector(axis));
}

xiiQuatd xiiBasisAxis::GetBasisRotationDouble_PosX(xiiBasisAxis::Enum axis)
{
  return xiiQuatd::MakeShortestRotation(xiiVec3d::MakeAxisX(), GetBasisVectorDouble(axis));
}

xiiQuat xiiBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  return xiiQuat::MakeShortestRotation(GetBasisVector(identity), GetBasisVector(axis));
}

xiiQuatd xiiBasisAxis::GetBasisRotationDouble(xiiBasisAxis::Enum identity, xiiBasisAxis::Enum axis)
{
  return xiiQuatd::MakeShortestRotation(GetBasisVectorDouble(identity), GetBasisVectorDouble(axis));
}

xiiBasisAxis::Enum xiiBasisAxis::GetOrthogonalAxis(Enum axis1, Enum axis2, bool bFlip)
{
  const xiiVec3 a1 = xiiBasisAxis::GetBasisVector(axis1);
  const xiiVec3 a2 = xiiBasisAxis::GetBasisVector(axis2);

  xiiVec3 c = a1.CrossRH(a2);

  if (bFlip)
    c = -c;

  if (c.IsEqual(xiiVec3::MakeAxisX(), 0.01f))
    return xiiBasisAxis::PositiveX;
  if (c.IsEqual(-xiiVec3::MakeAxisX(), 0.01f))
    return xiiBasisAxis::NegativeX;

  if (c.IsEqual(xiiVec3::MakeAxisY(), 0.01f))
    return xiiBasisAxis::PositiveY;
  if (c.IsEqual(-xiiVec3::MakeAxisY(), 0.01f))
    return xiiBasisAxis::NegativeY;

  if (c.IsEqual(xiiVec3::MakeAxisZ(), 0.01f))
    return xiiBasisAxis::PositiveZ;
  if (c.IsEqual(-xiiVec3::MakeAxisZ(), 0.01f))
    return xiiBasisAxis::NegativeZ;

  return axis1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiComparisonOperator, 1)
  XII_ENUM_CONSTANTS(xiiComparisonOperator::Equal, xiiComparisonOperator::NotEqual)
  XII_ENUM_CONSTANTS(xiiComparisonOperator::Less, xiiComparisonOperator::LessEqual)
  XII_ENUM_CONSTANTS(xiiComparisonOperator::Greater, xiiComparisonOperator::GreaterEqual)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiEasingFunction, 1)
 XII_ENUM_CONSTANT(xiiEasingFunction::ConstantZero),
 XII_ENUM_CONSTANT(xiiEasingFunction::ConstantQuarter),
 XII_ENUM_CONSTANT(xiiEasingFunction::ConstantHalf),
 XII_ENUM_CONSTANT(xiiEasingFunction::ConstantThreeFourths),
 XII_ENUM_CONSTANT(xiiEasingFunction::ConstantOne),
 XII_ENUM_CONSTANT(xiiEasingFunction::InLinear),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutLinear),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutLinear),
 XII_ENUM_CONSTANT(xiiEasingFunction::InSine),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutSine),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutSine),
 XII_ENUM_CONSTANT(xiiEasingFunction::InQuad),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutQuad),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutQuad),
 XII_ENUM_CONSTANT(xiiEasingFunction::InCubic),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutCubic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutCubic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InQuartic),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutQuartic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutQuartic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InQuintic),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutQuintic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutQuintic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InExpo),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutExpo),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutExpo),
 XII_ENUM_CONSTANT(xiiEasingFunction::InCirc),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutCirc),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutCirc),
 XII_ENUM_CONSTANT(xiiEasingFunction::InBack),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutBack),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutBack),
 XII_ENUM_CONSTANT(xiiEasingFunction::InElastic),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutElastic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutElastic),
 XII_ENUM_CONSTANT(xiiEasingFunction::InBounce),
 XII_ENUM_CONSTANT(xiiEasingFunction::OutBounce),
 XII_ENUM_CONSTANT(xiiEasingFunction::InOutBounce),
 XII_ENUM_CONSTANT(xiiEasingFunction::Conical),
 XII_ENUM_CONSTANT(xiiEasingFunction::FadeInHoldFadeOut),
 XII_ENUM_CONSTANT(xiiEasingFunction::FadeInFadeOut),
 XII_ENUM_CONSTANT(xiiEasingFunction::Bell),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
