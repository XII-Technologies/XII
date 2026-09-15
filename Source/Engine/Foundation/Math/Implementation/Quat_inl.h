/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type>::xiiQuatTemplate()
{
#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = xiiMath::NaN<Type>();
  x                  = TypeNaN;
  y                  = TypeNaN;
  z                  = TypeNaN;
  w                  = TypeNaN;
#endif
}

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type>::xiiQuatTemplate(Type inX, Type inY, Type inZ, Type inW) :
  x(inX), y(inY), z(inZ), w(inW)
{
}

template <typename Type>
XII_ALWAYS_INLINE const xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeIdentity()
{
  return xiiQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
XII_ALWAYS_INLINE xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeFromElements(Type inX, Type inY, Type inZ, Type inW)
{
  return xiiQuatTemplate<Type>(inX, inY, inZ, inW);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiQuatTemplate<Type>::SetIdentity()
{
  x = (Type)0;
  y = (Type)0;
  z = (Type)0;
  w = (Type)1;
}

template <typename Type>
xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeFromAxisAndAngle(const xiiVec3Template<Type>& vRotationAxis, xiiAngleTemplate<Type> angle)
{
  const xiiAngleTemplate<Type> halfAngle = angle * (Type)0.5;

  xiiVec3Template<Type> v = static_cast<Type>(xiiMath::Sin(halfAngle)) * vRotationAxis;
  Type                  w = xiiMath::Cos(halfAngle);

  return xiiQuatTemplate<Type>(v.x, v.y, v.z, w);
}

template <typename Type>
void xiiQuatTemplate<Type>::Normalize()
{
  XII_NAN_ASSERT(this);

  Type n = x * x + y * y + z * z + w * w;

  n = xiiMath::Invert(xiiMath::Sqrt(n));

  x *= n;
  y *= n;
  z *= n;
  w *= n;
}

template <typename Type>
void xiiQuatTemplate<Type>::GetRotationAxisAndAngle(xiiVec3Template<Type>& out_vAxis, xiiAngleTemplate<Type>& out_angle, Type fEpsilon) const
{
  XII_NAN_ASSERT(this);

  out_angle = (Type)2 * xiiMath::ACos(static_cast<Type>(w));

  const Type s = xiiMath::Sqrt(1 - w * w);

  if (s < fEpsilon)
  {
    out_vAxis.Set(1, 0, 0);
  }
  else
  {
    const Type ds = (Type)1 / s;
    out_vAxis.x   = x * ds;
    out_vAxis.y   = y * ds;
    out_vAxis.z   = z * ds;
  }
}

template <typename Type>
XII_FORCE_INLINE void xiiQuatTemplate<Type>::Invert()
{
  XII_NAN_ASSERT(this);

  *this = GetInverse();
}

template <typename Type>
XII_FORCE_INLINE const xiiQuatTemplate<Type> xiiQuatTemplate<Type>::GetInverse() const
{
  XII_NAN_ASSERT(this);

  return (xiiQuatTemplate(-x, -y, -z, w));
}

template <typename Type>
XII_FORCE_INLINE const xiiQuatTemplate<Type> xiiQuatTemplate<Type>::GetNegated() const
{
  XII_NAN_ASSERT(this);

  return (xiiQuatTemplate(-x, -y, -z, -w));
}

template <typename Type>
XII_FORCE_INLINE Type xiiQuatTemplate<Type>::Dot(const xiiQuatTemplate& rhs) const
{
  XII_NAN_ASSERT(this);
  XII_NAN_ASSERT(&rhs);

  return GetVectorPart().Dot(rhs.GetVectorPart()) + w * rhs.w;
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type> xiiQuatTemplate<Type>::Rotate(const xiiVec3Template<Type>& v) const
{
  return *this * v;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> operator*(const xiiQuatTemplate<Type>& q, const xiiVec3Template<Type>& v)
{
  xiiVec3Template<Type> t = q.GetVectorPart().CrossRH(v) * (Type)2;
  return v + q.w * t + q.GetVectorPart().CrossRH(t);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiQuatTemplate<Type> operator*(const xiiQuatTemplate<Type>& q1, const xiiQuatTemplate<Type>& q2)
{
  xiiQuatTemplate<Type> q;

  q.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);

  const xiiVec3Template<Type> v1 = q1.GetVectorPart();
  const xiiVec3Template<Type> v2 = q2.GetVectorPart();

  const xiiVec3Template<Type> vr = q1.w * v2 + q2.w * v1 + v1.CrossRH(v2);
  q.x                            = vr.x;
  q.y                            = vr.y;
  q.z                            = vr.z;

  return q;
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!GetVectorPart().IsValid())
    return false;
  if (!xiiMath::IsFinite(w))
    return false;

  Type n = x * x + y * y + z * z + w * w;

  return xiiMath::IsEqual(n, (Type)1, fEpsilon);
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsNaN() const
{
  return xiiMath::IsNaN(x) || xiiMath::IsNaN(y) || xiiMath::IsNaN(z) || xiiMath::IsNaN(w);
}

template <typename Type>
bool xiiQuatTemplate<Type>::IsEqualRotation(const xiiQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (GetVectorPart().IsEqual(qOther.GetVectorPart(), fEpsilon) && xiiMath::IsEqual(w, qOther.w, fEpsilon))
  {
    return true;
  }

  xiiVec3Template<Type>  vA1, vA2;
  xiiAngleTemplate<Type> A1, A2;

  GetRotationAxisAndAngle(vA1, A1);
  qOther.GetRotationAxisAndAngle(vA2, A2);

  if ((A1.IsEqualSimple(A2, xiiAngleTemplate<Type>::MakeFromDegree(static_cast<Type>(360.0f * fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, xiiAngleTemplate<Type>::MakeFromDegree(static_cast<Type>(360.0f * fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template <typename Type>
const xiiMat3Template<Type> xiiQuatTemplate<Type>::GetAsMat3() const
{
  XII_NAN_ASSERT(this);

  xiiMat3Template<Type> m;

  const Type fTx  = x + x;
  const Type fTy  = y + y;
  const Type fTz  = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

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

  const Type fTx  = x + x;
  const Type fTy  = y + y;
  const Type fTz  = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

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
xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeFromMat3(const xiiMat3Template<Type>& m)
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

  xiiQuatTemplate<Type> q;
  q.x = val[0];
  q.y = val[1];
  q.z = val[2];
  q.w = val[3];
  return q;
}

template <typename Type>
void xiiQuatTemplate<Type>::ReconstructFromMat3(const xiiMat3Template<Type>& mMat)
{
  const xiiVec3Template<Type> x = (mMat * xiiVec3Template<Type>(1, 0, 0)).GetNormalized();
  const xiiVec3Template<Type> y = (mMat * xiiVec3Template<Type>(0, 1, 0)).GetNormalized();
  const xiiVec3Template<Type> z = x.CrossRH(y);

  xiiMat3Template<Type> m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = xiiQuatTemplate<Type>::MakeFromMat3(m);
}

template <typename Type>
void xiiQuatTemplate<Type>::ReconstructFromMat4(const xiiMat4Template<Type>& mMat)
{
  const xiiVec3Template<Type> x = mMat.TransformDirection(xiiVec3Template<Type>(1, 0, 0)).GetNormalized();
  const xiiVec3Template<Type> y = mMat.TransformDirection(xiiVec3Template<Type>(0, 1, 0)).GetNormalized();
  const xiiVec3Template<Type> z = x.CrossRH(y);

  xiiMat3Template<Type> m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = xiiQuatTemplate<Type>::MakeFromMat3(m);
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template <typename Type>
xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeShortestRotation(const xiiVec3Template<Type>& vDirFrom, const xiiVec3Template<Type>& vDirTo)
{
  const xiiVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const xiiVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (xiiMath::IsEqual(fDot, (Type)1, (Type)0.0000001))
  {
    return MakeIdentity();
  }
  else if (xiiMath::IsEqual(fDot, (Type)-1, (Type)0.0000001)) // if both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, xiiVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (xiiMath::Abs(v0.Dot(xiiVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      return MakeFromAxisAndAngle(v0.CrossRH(xiiVec3Template<Type>(1, 0, 0)).GetNormalized(), xiiAngleTemplate<Type>::MakeFromRadian(xiiMath::Pi<Type>()));
    else
      return MakeFromAxisAndAngle(v0.CrossRH(xiiVec3Template<Type>(0, 1, 0)).GetNormalized(), xiiAngleTemplate<Type>::MakeFromRadian(xiiMath::Pi<Type>()));
  }

  const xiiVec3Template<Type> c = v0.CrossRH(v1);
  const Type                  d = v0.Dot(v1);
  const Type                  s = xiiMath::Sqrt(((Type)1 + d) * (Type)2);

  XII_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  const Type fOneDivS = 1.0f / s;

  xiiQuatTemplate<Type> q;
  q.x = c.x * fOneDivS;
  q.y = c.y * fOneDivS;
  q.z = c.z * fOneDivS;
  q.w = s / (Type)2;
  q.Normalize();

  return q;
}

template <typename Type>
xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeSlerp(const xiiQuatTemplate<Type>& qFrom, const xiiQuatTemplate<Type>& qTo, Type t)
{
  XII_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one    = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

  const Type fDot = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);

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
    xiiAngleTemplate<Type> theta = xiiMath::ACos((Type)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const Type                   iSinTheta = (Type)1 / xiiMath::Sqrt(one - (cosTheta * cosTheta));
    const xiiAngleTemplate<Type> tTheta    = static_cast<Type>(t) * theta;

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

  xiiQuatTemplate<Type> q;

  q.x = t0 * qFrom.x;
  q.y = t0 * qFrom.y;
  q.z = t0 * qFrom.z;
  q.w = t0 * qFrom.w;

  q.x += t1 * qTo.x;
  q.y += t1 * qTo.y;
  q.z += t1 * qTo.z;
  q.w += t1 * qTo.w;

  q.Normalize();

  return q;
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiQuatTemplate<Type>& q1, const xiiQuatTemplate<Type>& q2)
{
  return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
}

template <typename Type>
void xiiQuatTemplate<Type>::GetAsEulerAngles(xiiAngleTemplate<Type>& out_x, xiiAngleTemplate<Type>& out_y, xiiAngleTemplate<Type>& out_z) const
{
  // Taken from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  // and http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  // adapted to our convention (yaw->pitch->roll, ZYX order or 3-2-1 order)

  auto& yaw   = out_z;
  auto& pitch = out_y;
  auto& roll  = out_x;

  const double fSingularityTest      = w * y - z * x;
  const double fSingularityThreshold = 0.4999995;

  if (fSingularityTest > fSingularityThreshold) // singularity at north pole
  {
    yaw   = -static_cast<Type>(2) * xiiMath::ATan2(x, w);
    pitch = xiiAngleTemplate<Type>::MakeFromDegree(90.0f);
    roll  = xiiAngleTemplate<Type>::MakeFromDegree(0.0f);
  }
  else if (fSingularityTest < -fSingularityThreshold) // singularity at south pole
  {
    yaw   = static_cast<Type>(2) * xiiMath::ATan2(x, w);
    pitch = xiiAngleTemplate<Type>::MakeFromDegree(-90.0f);
    roll  = xiiAngleTemplate<Type>::MakeFromDegree(0.0f);
  }
  else
  {
    // yaw (z-axis rotation)
    const double siny = 2.0 * (w * z + x * y);
    const double cosy = 1.0 - 2.0 * (y * y + z * z);
    yaw               = xiiMath::ATan2((Type)siny, (Type)cosy);

    // pitch (y-axis rotation)
    pitch = xiiMath::ASin(static_cast<Type>(2) * (Type)fSingularityTest);

    // roll (x-axis rotation)
    const double sinr = 2.0 * (w * x + y * z);
    const double cosr = 1.0 - 2.0 * (x * x + y * y);
    roll              = xiiMath::ATan2((Type)sinr, (Type)cosr);
  }
}

template <typename Type>
xiiQuatTemplate<Type> xiiQuatTemplate<Type>::MakeFromEulerAngles(const xiiAngleTemplate<Type>& x, const xiiAngleTemplate<Type>& y, const xiiAngleTemplate<Type>& z)
{
  /// Taken from here (yaw->pitch->roll, ZYX order or 3-2-1 order):
  /// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const auto&  yaw   = z;
  const auto&  pitch = y;
  const auto&  roll  = x;
  const double cy    = xiiMath::Cos(yaw * (Type)0.5);
  const double sy    = xiiMath::Sin(yaw * (Type)0.5);
  const double cp    = xiiMath::Cos(pitch * (Type)0.5);
  const double sp    = xiiMath::Sin(pitch * (Type)0.5);
  const double cr    = xiiMath::Cos(roll * (Type)0.5);
  const double sr    = xiiMath::Sin(roll * (Type)0.5);

  xiiQuatTemplate<Type> q;
  q.w = (Type)(cy * cp * cr + sy * sp * sr);
  q.x = (Type)(cy * cp * sr - sy * sp * cr);
  q.y = (Type)(cy * sp * cr + sy * cp * sr);
  q.z = (Type)(sy * cp * cr - cy * sp * sr);
  return q;
}
