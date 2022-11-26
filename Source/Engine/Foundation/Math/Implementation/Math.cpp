#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>

// Default are D3D convention before a renderer is initialized.
xiiClipSpaceDepthRange::Enum xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
xiiClipSpaceYMode::Enum      xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

xiiHandedness::Enum xiiHandedness::Default = xiiHandedness::LeftHanded;

bool xiiMath::IsPowerOf(xiiInt32 value, xiiInt32 base)
{
  if (value == 1)
    return true;

  while (value > base)
  {
    if (value % base == 0)
      value /= base;
    else
      return false;
  }

  return (value == base);
}

xiiUInt32 xiiMath::PowerOfTwo_Floor(xiiUInt32 npot)
{
  if (IsPowerOf2(npot))
    return (npot);

  for (xiiUInt32 i = 1; i <= (sizeof(npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << i);
  }

  return (1);
}

xiiUInt32 xiiMath::PowerOfTwo_Ceil(xiiUInt32 npot)
{
  if (IsPowerOf2(npot))
    return (npot);

  for (xiiUInt32 i = 1; i <= (sizeof(npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return npot << (i + 1u);
    }
  }

  return (1u);
}


xiiUInt32 xiiMath::GreatestCommonDivisor(xiiUInt32 a, xiiUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return a;
  }
  if (b == 0)
  {
    return b;
  }

  xiiUInt32 shift = FirstBitLow(a | b);
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
  return a << shift;
}

xiiResult xiiMath::TryMultiply32(xiiUInt32& out_Result, xiiUInt32 a, xiiUInt32 b, xiiUInt32 c, xiiUInt32 d)
{
  xiiUInt64 result = static_cast<xiiUInt64>(a) * static_cast<xiiUInt64>(b);

  if (result > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  result *= static_cast<xiiUInt64>(c);

  if (result > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  result *= static_cast<xiiUInt64>(d);

  if (result > 0xFFFFFFFFllu)
  {
    return XII_FAILURE;
  }

  out_Result = static_cast<xiiUInt32>(result & 0xFFFFFFFFllu);
  return XII_SUCCESS;
}

xiiUInt32 xiiMath::SafeMultiply32(xiiUInt32 a, xiiUInt32 b, xiiUInt32 c, xiiUInt32 d)
{
  xiiUInt32 result = 0;
  if (TryMultiply32(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  XII_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds UInt32 range.", a, b, c, d);
  std::terminate();
  return 0;
}

xiiResult xiiMath::TryMultiply64(xiiUInt64& out_Result, xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, xiiUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_Result = 0;
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

  out_Result = abcd;
  return XII_SUCCESS;
}

xiiUInt64 xiiMath::SafeMultiply64(xiiUInt64 a, xiiUInt64 b, xiiUInt64 c, xiiUInt64 d)
{
  xiiUInt64 result = 0;
  if (TryMultiply64(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  XII_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds xiiUInt64 range.", a, b, c, d);
  std::terminate();
  return 0;
}

#if XII_ENABLED(XII_PLATFORM_32BIT)
size_t xiiMath::SafeConvertToSizeT(xiiUInt64 uiValue)
{
  size_t result = 0;
  if (TryConvertToSizeT(result, uiValue).Succeeded())
  {
    return result;
  }

  XII_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
  return 0;
}
#endif

void xiiAngle::NormalizeRange()
{
  const float fTwoPi = 2.0f * Pi<float>();

  const float fTwoPiTen = 10.0f * Pi<float>();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    m_fRadian = xiiMath::Mod(m_fRadian, fTwoPi);
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < 0.0f)
  {
    m_fRadian += fTwoPi;
  }
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
      return xiiVec3::ZeroVector();
  }
}

xiiMat3 xiiBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum upDir, float fUniformScale /*= 1.0f*/, float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  xiiMat3 mResult;
  mResult.SetRow(0, xiiBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, xiiBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, xiiBasisAxis::GetBasisVector(upDir) * fUniformScale * fScaleZ);

  return mResult;
}


xiiQuat xiiBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  xiiQuat rotAxis;
  switch (axis)
  {
    case xiiBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case xiiBasisAxis::PositiveY:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(90));
      break;
    case xiiBasisAxis::PositiveZ:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(-90));
      break;
    case xiiBasisAxis::NegativeX:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(180));
      break;
    case xiiBasisAxis::NegativeY:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(-90));
      break;
    case xiiBasisAxis::NegativeZ:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      break;
  }

  return rotAxis;
}

xiiQuat xiiBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  xiiQuat rotId;
  switch (identity)
  {
    case xiiBasisAxis::PositiveX:
      rotId.SetIdentity();
      break;
    case xiiBasisAxis::PositiveY:
      rotId.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(-90));
      break;
    case xiiBasisAxis::PositiveZ:
      rotId.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      break;
    case xiiBasisAxis::NegativeX:
      rotId.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(180));
      break;
    case xiiBasisAxis::NegativeY:
      rotId.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(90));
      break;
    case xiiBasisAxis::NegativeZ:
      rotId.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      break;
  }

  xiiQuat rotAxis;
  switch (axis)
  {
    case xiiBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case xiiBasisAxis::PositiveY:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(90));
      break;
    case xiiBasisAxis::PositiveZ:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(-90));
      break;
    case xiiBasisAxis::NegativeX:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(180));
      break;
    case xiiBasisAxis::NegativeY:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(-90));
      break;
    case xiiBasisAxis::NegativeZ:
      rotAxis.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(90));
      break;
  }

  return rotAxis * rotId;
}

xiiBasisAxis::Enum xiiBasisAxis::GetOrthogonalAxis(Enum axis1, Enum axis2, bool flip)
{
  const xiiVec3 a1 = xiiBasisAxis::GetBasisVector(axis1);
  const xiiVec3 a2 = xiiBasisAxis::GetBasisVector(axis2);

  xiiVec3 c = a1.CrossRH(a2);

  if (flip)
    c = -c;

  if (c.IsEqual(xiiVec3::UnitXAxis(), 0.01f))
    return xiiBasisAxis::PositiveX;
  if (c.IsEqual(-xiiVec3::UnitXAxis(), 0.01f))
    return xiiBasisAxis::NegativeX;

  if (c.IsEqual(xiiVec3::UnitYAxis(), 0.01f))
    return xiiBasisAxis::PositiveY;
  if (c.IsEqual(-xiiVec3::UnitYAxis(), 0.01f))
    return xiiBasisAxis::NegativeY;

  if (c.IsEqual(xiiVec3::UnitZAxis(), 0.01f))
    return xiiBasisAxis::PositiveZ;
  if (c.IsEqual(-xiiVec3::UnitZAxis(), 0.01f))
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
// clang-format on

// static
bool xiiComparisonOperator::Compare(xiiComparisonOperator::Enum cmp, double f1, double f2)
{
  switch (cmp)
  {
    case xiiComparisonOperator::Equal:
      return f1 == f2;
    case xiiComparisonOperator::NotEqual:
      return f1 != f2;
    case xiiComparisonOperator::Less:
      return f1 < f2;
    case xiiComparisonOperator::LessEqual:
      return f1 <= f2;
    case xiiComparisonOperator::Greater:
      return f1 > f2;
    case xiiComparisonOperator::GreaterEqual:
      return f1 >= f2;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return false;
}


XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
