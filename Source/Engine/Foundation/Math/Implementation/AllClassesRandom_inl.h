#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomPointInSphere(xiiRandom& inout_rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = inout_rng.DoubleMinMax(-1, 1);
    py = inout_rng.DoubleMinMax(-1, 1);
    pz = inout_rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // Prevent from being the exact center

  return xiiVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDirection(xiiRandom& inout_rng)
{
  xiiVec3Template<Type> vec = MakeRandomPointInSphere(inout_rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationX(xiiRandom& inout_rng, const xiiAngle& maxDeviation)
{
  const double twoPi = 2.0 * xiiMath::Pi<double>();

  const double cosAngle = xiiMath::Cos(maxDeviation);

  const double   x       = inout_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const xiiAngle phi     = xiiAngle::MakeFromRadian((float)(inout_rng.DoubleZeroToOneInclusive() * twoPi));
  const double   invSqrt = xiiMath::Sqrt(1 - (x * x));
  const double   y       = invSqrt * xiiMath::Cos(phi);
  const double   z       = invSqrt * xiiMath::Sin(phi);

  return xiiVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationX(xiiRandom& inout_rng, const xiiAngled& maxDeviation)
{
  const double twoPi = 2.0 * xiiMath::Pi<double>();

  const double cosAngle = xiiMath::Cos(maxDeviation);

  const double    x       = inout_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const xiiAngled phi     = xiiAngled::MakeFromRadian(inout_rng.DoubleZeroToOneInclusive() * twoPi);
  const double    invSqrt = xiiMath::Sqrt(1 - (x * x));
  const double    y       = invSqrt * xiiMath::Cos(phi);
  const double    z       = invSqrt * xiiMath::Sin(phi);

  return xiiVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationY(xiiRandom& inout_rng, const xiiAngle& maxDeviation)
{
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationY(xiiRandom& inout_rng, const xiiAngled& maxDeviation)
{
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationZ(xiiRandom& inout_rng, const xiiAngle& maxDeviation)
{
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviationZ(xiiRandom& inout_rng, const xiiAngled& maxDeviation)
{
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviation(xiiRandom& inout_rng, const xiiAngle& maxDeviation, const xiiVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  xiiQuatTemplate<Type> qRotXtoDir = xiiQuatTemplate<Type>::MakeShortestRotation(xiiVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}

template <typename Type>
XII_IMPLEMENT_IF_FLOAT_TYPE xiiVec3Template<Type> xiiVec3Template<Type>::MakeRandomDeviation(xiiRandom& inout_rng, const xiiAngled& maxDeviation, const xiiVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  xiiQuatTemplate<Type> qRotXtoDir = xiiQuatTemplate<Type>::MakeShortestRotation(xiiVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  xiiVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
