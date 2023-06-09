#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type>::xiiQuatTemplate()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  w                  = TypeNaN;
#endif
}

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type>::xiiQuatTemplate(Type inX, Type inY, Type inZ, Type inW) :
  v(inX, inY, inZ), w(inW)
{
}

template <typename Type>
XII_ALWAYS_INLINE const xiiQuatTemplate<Type> xiiQuatTemplate<Type>::IdentityQuaternion()
{
  return xiiQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiQuatTemplate<Type>::SetElements(Type inX, Type inY, Type inZ, Type inW)
{
  v.Set(inX, inY, inZ);
  w = inW;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiQuatTemplate<Type>::SetIdentity()
{
  v.SetZero();
  w = (Type)1;
}

template <typename Type>
void xiiQuatTemplate<Type>::SetFromAxisAndAngle(const xiiVec3Template<Type>& vRotationAxis, xiiAngleTemplate<Type> angle)
{
  const xiiAngleTemplate<Type> halfAngle = angle * (Type)0.5;

  v = static_cast<Type>(xiiMath::Sin(halfAngle)) * vRotationAxis;
  w = xiiMath::Cos(halfAngle);
}

template <typename Type>
void xiiQuatTemplate<Type>::Normalize()
{
  XII_NAN_ASSERT(this);

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  n = xiiMath::Invert(xiiMath::Sqrt(n));

  v *= n;
  w *= n;
}

template <typename Type>
xiiResult xiiQuatTemplate<Type>::GetRotationAxisAndAngle(xiiVec3Template<Type>& ref_vAxis, xiiAngleTemplate<Type>& ref_angle, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  const xiiAngleTemplate<Type> acos = xiiMath::ACos(static_cast<Type>(w));
  const Type                   d    = xiiMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1, 0, 0);
  }
  else
  {
    ref_vAxis = (v / static_cast<Type>(d));
  }

  ref_angle = acos * 2.0f;

  return XII_SUCCESS;
}

template <typename Type>
XII_FORCE_INLINE void xiiQuatTemplate<Type>::Invert()
{
  XII_NAN_ASSERT(this);

  *this = -(*this);
}

template <typename Type>
XII_FORCE_INLINE const xiiQuatTemplate<Type> xiiQuatTemplate<Type>::operator-() const
{
  XII_NAN_ASSERT(this);

  return (xiiQuatTemplate(-v.x, -v.y, -v.z, w));
}

template <typename Type>
XII_FORCE_INLINE Type xiiQuatTemplate<Type>::Dot(const xiiQuatTemplate& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return v.Dot(rhs.v) + w * rhs.w;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> operator*(const xiiQuatTemplate<Type>& q, const xiiVec3Template<Type>& v)
{
  xiiVec3Template<Type> t = q.v.CrossRH(v) * (Type)2;
  return v + q.w * t + q.v.CrossRH(t);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiQuatTemplate<Type> operator*(const xiiQuatTemplate<Type>& q1, const xiiQuatTemplate<Type>& q2)
{
  xiiQuatTemplate<Type> q;

  q.w = q1.w * q2.w - q1.v.Dot(q2.v);
  q.v = q1.w * q2.v + q2.w * q1.v + q1.v.CrossRH(q2.v);

  return (q);
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!v.IsValid())
    return false;
  if (!xiiMath::IsFinite(w))
    return false;

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  return (xiiMath::IsEqual(n, (Type)1, fEpsilon));
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsNaN() const
{
  return v.IsNaN() || xiiMath::IsNaN(w);
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsEqualRotation(const xiiQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (v.IsEqual(qOther.v, xiiMath::DefaultEpsilon<Type>()) && xiiMath::IsEqual(w, qOther.w, xiiMath::DefaultEpsilon<Type>()))
  {
    return true;
  }

  xiiVec3Template<Type> vA1, vA2;
  xiiAngle              A1, A2;

  if (GetRotationAxisAndAngle(vA1, A1) == XII_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, A2) == XII_FAILURE)
    return false;

  if ((A1.IsEqualSimple(A2, xiiAngle::Degree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, xiiAngle::Degree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template <typename Type>
const xiiMat3Template<Type> xiiQuatTemplate<Type>::GetAsMat3() const
{
  XII_NAN_ASSERT(this);

  xiiMat3Template<Type> m;

  const Type fTx  = v.x + v.x;
  const Type fTy  = v.y + v.y;
  const Type fTz  = v.z + v.z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * v.x;
  const Type fTxy = fTy * v.x;
  const Type fTxz = fTz * v.x;
  const Type fTyy = fTy * v.y;
  const Type fTyz = fTz * v.y;
  const Type fTzz = fTz * v.z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  return m;
}

template <typename Type>
const xiiMat4Template<Type> xiiQuatTemplate<Type>::GetAsMat4() const
{
  XII_NAN_ASSERT(this);

  xiiMat4Template<Type> m;

  const Type fTx  = v.x + v.x;
  const Type fTy  = v.y + v.y;
  const Type fTz  = v.z + v.z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * v.x;
  const Type fTxy = fTy * v.x;
  const Type fTxz = fTz * v.x;
  const Type fTyy = fTy * v.y;
  const Type fTyz = fTz * v.y;
  const Type fTzz = fTz * v.z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(3, 0) = (Type)0;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(3, 1) = (Type)0;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  m.Element(3, 2) = (Type)0;
  m.Element(0, 3) = (Type)0;
  m.Element(1, 3) = (Type)0;
  m.Element(2, 3) = (Type)0;
  m.Element(3, 3) = (Type)1;
  return m;
}

template <typename Type>
void xiiQuatTemplate<Type>::SetFromMat3(const xiiMat3Template<Type>& m)
{
  XII_NAN_ASSERT(&m);

  const Type trace = m.Element(0, 0) + m.Element(1, 1) + m.Element(2, 2);
  const Type half  = (Type)0.5;

  Type val[4];

  if (trace > (Type)0)
  {
    Type s = xiiMath::Sqrt(trace + (Type)1);
    Type t = half / s;

    val[0] = (m.Element(1, 2) - m.Element(2, 1)) * t;
    val[1] = (m.Element(2, 0) - m.Element(0, 2)) * t;
    val[2] = (m.Element(0, 1) - m.Element(1, 0)) * t;

    val[3] = half * s;
  }
  else
  {
    const xiiInt32 next[] = {1, 2, 0};
    xiiInt32       i      = 0;

    if (m.Element(1, 1) > m.Element(0, 0))
      i = 1;

    if (m.Element(2, 2) > m.Element(i, i))
      i = 2;

    xiiInt32 j = next[i];
    xiiInt32 k = next[j];

    Type s = xiiMath::Sqrt(m.Element(i, i) - (m.Element(j, j) + m.Element(k, k)) + (Type)1);
    Type t = half / s;

    val[i] = half * s;
    val[3] = (m.Element(j, k) - m.Element(k, j)) * t;
    val[j] = (m.Element(i, j) + m.Element(j, i)) * t;
    val[k] = (m.Element(i, k) + m.Element(k, i)) * t;
  }

  v.x = val[0];
  v.y = val[1];
  v.z = val[2];
  w   = val[3];
}

template <typename Type>
void xiiQuatTemplate<Type>::ReconstructFromMat3(const xiiMat3Template<Type>& mMat)
{
  const xiiVec3 x = (mMat * xiiVec3(1, 0, 0)).GetNormalized();
  const xiiVec3 y = (mMat * xiiVec3(0, 1, 0)).GetNormalized();
  const xiiVec3 z = x.CrossRH(y);

  xiiMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  SetFromMat3(m);
}

template <typename Type>
void xiiQuatTemplate<Type>::ReconstructFromMat4(const xiiMat4Template<Type>& mMat)
{
  const xiiVec3 x = mMat.TransformDirection(xiiVec3(1, 0, 0)).GetNormalized();
  const xiiVec3 y = mMat.TransformDirection(xiiVec3(0, 1, 0)).GetNormalized();
  const xiiVec3 z = x.CrossRH(y);

  xiiMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  SetFromMat3(m);
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template <typename Type>
void xiiQuatTemplate<Type>::SetShortestRotation(const xiiVec3Template<Type>& vDirFrom, const xiiVec3Template<Type>& vDirTo)
{
  const xiiVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const xiiVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (xiiMath::IsEqual(fDot, (Type)1, xiiMath::SmallEpsilon<Type>()))
  {
    SetIdentity();
    return;
  }
  else if (xiiMath::IsEqual(fDot, (Type)-1, xiiMath::SmallEpsilon<Type>())) // If both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, xiiVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (xiiMath::Abs(v0.Dot(xiiVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      SetFromAxisAndAngle(v0.CrossRH(xiiVec3Template<Type>(1, 0, 0)).GetNormalized(), xiiAngleTemplate<Type>::Radian(xiiMath::Pi<Type>()));
    else
      SetFromAxisAndAngle(v0.CrossRH(xiiVec3Template<Type>(0, 1, 0)).GetNormalized(), xiiAngleTemplate<Type>::Radian(xiiMath::Pi<Type>()));

    return;
  }

  const xiiVec3Template<Type> c = v0.CrossRH(v1);
  const Type                  d = v0.Dot(v1);
  const Type                  s = xiiMath::Sqrt(((Type)1 + d) * (Type)2);

  XII_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  v = c / s;
  w = s / (Type)2;

  Normalize();
}

template <typename Type>
void xiiQuatTemplate<Type>::SetSlerp(const xiiQuatTemplate<Type>& qFrom, const xiiQuatTemplate<Type>& qTo, Type t)
{
  XII_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one    = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

  const Type fDot = (qFrom.v.x * qTo.v.x + qFrom.v.y * qTo.v.y + qFrom.v.z * qTo.v.z + qFrom.w * qTo.w);

  Type cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < (Type)0)
  {
    bFlipSign = true;
    cosTheta  = -cosTheta;
  }

  Type t0, t1;

  if (cosTheta < qdelta)
  {
    xiiAngleTemplate<Type> theta = xiiMath::ACos(cosTheta);

    // Use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const Type                   iSinTheta = (Type)1 / xiiMath::Sqrt(one - (cosTheta * cosTheta));
    const xiiAngleTemplate<Type> tTheta    = t * theta;

    Type s0 = xiiMath::Sin(theta - tTheta);
    Type s1 = xiiMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  v.x = t0 * qFrom.v.x;
  v.y = t0 * qFrom.v.y;
  v.z = t0 * qFrom.v.z;
  w   = t0 * qFrom.w;

  v.x += t1 * qTo.v.x;
  v.y += t1 * qTo.v.y;
  v.z += t1 * qTo.v.z;
  w += t1 * qTo.w;

  Normalize();
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiQuatTemplate<Type>& q1, const xiiQuatTemplate<Type>& q2)
{
  return q1.v.IsIdentical(q2.v) && q1.w == q2.w;
}

template <typename Type>
XII_ALWAYS_INLINE bool operator!=(const xiiQuatTemplate<Type>& q1, const xiiQuatTemplate<Type>& q2)
{
  return !(q1 == q2);
}

template <typename Type>
void xiiQuatTemplate<Type>::GetAsEulerAngles(xiiAngleTemplate<Type>& out_x, xiiAngleTemplate<Type>& out_y, xiiAngleTemplate<Type>& out_z) const
{
  XII_NAN_ASSERT(this);

  ///\test This is new.

  /// This is adapted from https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  /// It is also used in the OZZ Animation Library's "ToEuler" conversion.

  struct Q
  {
    Type x, y, z, w;
  };

  const Q _q{v.x, v.y, v.z, w};

  const Type kPi_2 = (Type)1.5707963267948966192313216916398;

  const Type sqw = _q.w * _q.w;
  const Type sqx = _q.x * _q.x;
  const Type sqy = _q.y * _q.y;
  const Type sqz = _q.z * _q.z;
  // If normalized is one, otherwise is correction factor.
  const Type            unit = sqx + sqy + sqz + sqw;
  const Type            test = _q.x * _q.y + _q.z * _q.w;
  xiiVec3Template<Type> euler;

  if (test > 0.499f * unit) // Singularity at the North Pole.
  {
    euler.x = 2.0f * std::atan2(_q.x, _q.w);
    euler.y = kPi_2;
    euler.z = 0;
  }
  else if (test < -0.499f * unit) // Singularity at the South Pole.
  {
    euler.x = -2 * std::atan2(_q.x, _q.w);
    euler.y = -kPi_2;
    euler.z = 0;
  }
  else
  {
    euler.x = std::atan2(2.0f * _q.y * _q.w - 2.0f * _q.x * _q.z, sqx - sqy - sqz + sqw);
    euler.y = std::asin(2.0f * test / unit);
    euler.z = std::atan2(2.0f * _q.x * _q.w - 2.0f * _q.y * _q.z, -sqx + sqy - sqz + sqw);
  }

  out_x.SetRadian(euler.z);
  out_y.SetRadian(euler.x);
  out_z.SetRadian(euler.y);
}

template <typename Type>
void xiiQuatTemplate<Type>::SetFromEulerAngles(const xiiAngleTemplate<Type>& x, const xiiAngleTemplate<Type>& y, const xiiAngleTemplate<Type>& z)
{
  /// \test This is new

  /// Taken from here (roll->pitch->yaw, x->y->z order):
  /// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const auto&  yaw   = z;
  const auto&  pitch = y;
  const auto&  roll  = x;
  const double cy    = xiiMath::Cos(yaw * (Type)0.5);
  const double sy    = xiiMath::Sin(yaw * (Type)0.5);
  const double cr    = xiiMath::Cos(roll * (Type)0.5);
  const double sr    = xiiMath::Sin(roll * (Type)0.5);
  const double cp    = xiiMath::Cos(pitch * (Type)0.5);
  const double sp    = xiiMath::Sin(pitch * (Type)0.5);

  w   = (Type)(cy * cr * cp + sy * sr * sp);
  v.x = (Type)(cy * sr * cp - sy * cr * sp);
  v.y = (Type)(cy * cr * sp + sy * sr * cp);
  v.z = (Type)(sy * cr * cp - cy * sr * sp);
}
