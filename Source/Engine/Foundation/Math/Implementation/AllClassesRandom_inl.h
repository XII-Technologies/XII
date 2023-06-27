#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomPointInSphere(xiiRandom& ref_rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = ref_rng.DoubleMinMax(-1, 1);
    py = ref_rng.DoubleMinMax(-1, 1);
    pz = ref_rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // Prevent from being the exact center

  return xiiVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDirection(xiiRandom& ref_rng)
{
  xiiVec3Template<Type> vec = CreateRandomPointInSphere(ref_rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationX(xiiRandom& ref_rng, const xiiAngle& maxDeviation)
{
  const double twoPi = 2.0 * xiiMath::Pi<double>();

  const double cosAngle = xiiMath::Cos(maxDeviation);

  const double   x       = ref_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const xiiAngle phi     = xiiAngle::Radian((float)(ref_rng.DoubleZeroToOneInclusive() * twoPi));
  const double   invSqrt = xiiMath::Sqrt(1 - (x * x));
  const double   y       = invSqrt * xiiMath::Cos(phi);
  const double   z       = invSqrt * xiiMath::Sin(phi);

  return xiiVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationX(xiiRandom& ref_rng, const xiiAngled& maxDeviation)
{
  const double twoPi = 2.0 * xiiMath::Pi<double>();

  const double cosAngle = xiiMath::Cos(maxDeviation);

  const double    x       = ref_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const xiiAngled phi     = xiiAngled::Radian(ref_rng.DoubleZeroToOneInclusive() * twoPi);
  const double    invSqrt = xiiMath::Sqrt(1 - (x * x));
  const double    y       = invSqrt * xiiMath::Cos(phi);
  const double    z       = invSqrt * xiiMath::Sin(phi);

  return xiiVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationY(xiiRandom& ref_rng, const xiiAngle& maxDeviation)
{
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationY(xiiRandom& ref_rng, const xiiAngled& maxDeviation)
{
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationZ(xiiRandom& ref_rng, const xiiAngle& maxDeviation)
{
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviationZ(xiiRandom& ref_rng, const xiiAngled& maxDeviation)
{
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  xiiMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviation(xiiRandom& ref_rng, const xiiAngle& maxDeviation, const xiiVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  xiiQuatTemplate<Type> qRotXtoDir;
  qRotXtoDir.SetShortestRotation(xiiVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}

template <typename Type>
xiiVec3Template<Type> xiiVec3Template<Type>::CreateRandomDeviation(xiiRandom& ref_rng, const xiiAngled& maxDeviation, const xiiVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  xiiQuatTemplate<Type> qRotXtoDir;
  qRotXtoDir.SetShortestRotation(xiiVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  xiiVec3Template<Type> vec = CreateRandomDeviationX(ref_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
