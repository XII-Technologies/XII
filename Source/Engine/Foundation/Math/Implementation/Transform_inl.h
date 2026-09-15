/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Transform.h>

template <typename Type>
inline xiiTransformTemplate<Type>::xiiTransformTemplate(const xiiVec3Template<Type>& vPosition, const xiiQuatTemplate<Type>& qRotation, const xiiVec3Template<Type>& vScale) :
  m_vPosition(vPosition), m_qRotation(qRotation), m_vScale(vScale)
{
}

template <typename Type>
inline xiiTransformTemplate<Type> xiiTransformTemplate<Type>::Make(const xiiVec3Template<Type>& vPosition, const xiiQuatTemplate<Type>& qRotation /*= xiiQuatTemplate<Type>::IdentityQuaternion()*/, const xiiVec3Template<Type>& vScale /*= xiiVec3Template<Type>(1)*/)
{
  xiiTransformTemplate<Type> res;
  res.m_vPosition = vPosition;
  res.m_qRotation = qRotation;
  res.m_vScale    = vScale;
  return res;
}

template <typename Type>
inline xiiTransformTemplate<Type> xiiTransformTemplate<Type>::MakeIdentity()
{
  xiiTransformTemplate<Type> res;
  res.m_vPosition.SetZero();
  res.m_qRotation = xiiQuatTemplate<Type>::MakeIdentity();
  res.m_vScale.Set(1.0f);
  return res;
}

template <typename Type>
xiiTransformTemplate<Type> xiiTransformTemplate<Type>::MakeFromMat4(const xiiMat4Template<Type>& mMat)
{
  xiiMat3Template<Type> mRot = mMat.GetRotationalPart();

  xiiTransformTemplate<Type> res;
  res.m_vPosition = mMat.GetTranslationVector();
  res.m_vScale    = mRot.GetScalingFactors();
  mRot.SetScalingFactors(xiiVec3Template<Type>(1)).IgnoreResult();
  res.m_qRotation = xiiQuatTemplate<Type>::MakeFromMat3(mRot);
  return res;
}

template <typename Type>
xiiTransformTemplate<Type> xiiTransformTemplate<Type>::MakeLocalTransform(const xiiTransformTemplate& globalTransformParent, const xiiTransformTemplate& globalTransformChild)
{
  const auto invRot   = globalTransformParent.m_qRotation.GetInverse();
  const auto invScale = xiiVec3Template<Type>(1).CompDiv(globalTransformParent.m_vScale);

  xiiTransformTemplate<Type> res;
  res.m_vPosition = (invRot * (globalTransformChild.m_vPosition - globalTransformParent.m_vPosition)).CompMul(invScale);
  res.m_qRotation = invRot * globalTransformChild.m_qRotation;
  res.m_vScale    = invScale.CompMul(globalTransformChild.m_vScale);
  return res;
}

template <typename Type>
XII_ALWAYS_INLINE xiiTransformTemplate<Type> xiiTransformTemplate<Type>::MakeGlobalTransform(const xiiTransformTemplate& globalTransformParent, const xiiTransformTemplate& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiTransformTemplate<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
XII_ALWAYS_INLINE Type xiiTransformTemplate<Type>::GetMaxScale() const
{
  auto absScale = m_vScale.Abs();
  return xiiMath::Max(absScale.x, xiiMath::Max(absScale.y, absScale.z));
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiTransformTemplate<Type>::ContainsNegativeScale() const
{
  return (m_vScale.x * m_vScale.y * m_vScale.z) < 0.0f;
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiTransformTemplate<Type>::ContainsUniformScale() const
{
  const Type fEpsilon = xiiMath::DefaultEpsilon<Type>();
  return xiiMath::IsEqual(m_vScale.x, m_vScale.y, fEpsilon) && xiiMath::IsEqual(m_vScale.x, m_vScale.z, fEpsilon);
}

template <typename Type>
inline bool xiiTransformTemplate<Type>::IsIdentical(const xiiTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && (m_qRotation == rhs.m_qRotation) && m_vScale.IsIdentical(rhs.m_vScale);
}

template <typename Type>
inline bool xiiTransformTemplate<Type>::IsEqual(const xiiTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_qRotation.IsEqualRotation(rhs.m_qRotation, fEpsilon) && m_vScale.IsEqual(rhs.m_vScale, fEpsilon);
}

template <typename Type>
inline bool xiiTransformTemplate<Type>::IsValid() const
{
  return m_vPosition.IsValid() && m_qRotation.IsValid(0.005f) && m_vScale.IsValid();
}

template <typename Type>
XII_ALWAYS_INLINE const xiiMat4Template<Type> xiiTransformTemplate<Type>::GetAsMat4() const
{
  xiiMat4Template<Type> result = m_qRotation.GetAsMat4();

  result.m_fElementsCM[0] *= m_vScale.x;
  result.m_fElementsCM[1] *= m_vScale.x;
  result.m_fElementsCM[2] *= m_vScale.x;

  result.m_fElementsCM[4] *= m_vScale.y;
  result.m_fElementsCM[5] *= m_vScale.y;
  result.m_fElementsCM[6] *= m_vScale.y;

  result.m_fElementsCM[8] *= m_vScale.z;
  result.m_fElementsCM[9] *= m_vScale.z;
  result.m_fElementsCM[10] *= m_vScale.z;

  result.m_fElementsCM[12] = m_vPosition.x;
  result.m_fElementsCM[13] = m_vPosition.y;
  result.m_fElementsCM[14] = m_vPosition.z;

  return result;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiTransformTemplate<Type>::operator+=(const xiiVec3Template<Type>& v)
{
  m_vPosition += v;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiTransformTemplate<Type>::operator-=(const xiiVec3Template<Type>& v)
{
  m_vPosition -= v;
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type> xiiTransformTemplate<Type>::TransformPosition(const xiiVec3Template<Type>& v) const
{
  const auto scaled  = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return m_vPosition + rotated;
}

template <typename Type>
XII_ALWAYS_INLINE xiiVec3Template<Type> xiiTransformTemplate<Type>::TransformDirection(const xiiVec3Template<Type>& v) const
{
  const auto scaled  = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return rotated;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiTransformTemplate<Type> operator*(const xiiQuatTemplate<Type>& q, const xiiTransformTemplate<Type>& t)
{
  xiiTransformTemplate<Type> r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = q * t.m_qRotation;
  r.m_vScale    = t.m_vScale;

  return r;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiTransformTemplate<Type> operator*(const xiiTransformTemplate<Type>& t, const xiiQuatTemplate<Type>& q)
{
  xiiTransformTemplate<Type> r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = t.m_qRotation * q;
  r.m_vScale    = t.m_vScale;

  return r;
}

template <typename Type>
XII_ALWAYS_INLINE const xiiTransformTemplate<Type> operator+(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v)
{
  return xiiTransformTemplate<Type>(t.m_vPosition + v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiTransformTemplate<Type> operator-(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v)
{
  return xiiTransformTemplate<Type>(t.m_vPosition - v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
XII_ALWAYS_INLINE const xiiVec3Template<Type> operator*(const xiiTransformTemplate<Type>& t, const xiiVec3Template<Type>& v)
{
  return t.TransformPosition(v);
}

template <typename Type>
inline const xiiTransformTemplate<Type> operator*(const xiiTransformTemplate<Type>& t1, const xiiTransformTemplate<Type>& t2)
{
  xiiTransformTemplate<Type> t;

  t.m_vPosition = (t1.m_qRotation * t2.m_vPosition.CompMul(t1.m_vScale)) + t1.m_vPosition;
  t.m_qRotation = t1.m_qRotation * t2.m_qRotation;
  t.m_vScale    = t1.m_vScale.CompMul(t2.m_vScale);

  return t;
}

template <typename Type>
XII_ALWAYS_INLINE bool operator==(const xiiTransformTemplate<Type>& t1, const xiiTransformTemplate<Type>& t2)
{
  return t1.IsIdentical(t2);
}

template <typename Type>
XII_ALWAYS_INLINE void xiiTransformTemplate<Type>::Invert()
{
  (*this) = GetInverse();
}

template <typename Type>
inline const xiiTransformTemplate<Type> xiiTransformTemplate<Type>::GetInverse() const
{
  const auto invRot   = m_qRotation.GetInverse();
  const auto invScale = xiiVec3Template<Type>(1).CompDiv(m_vScale);
  const auto invPos   = invRot * (invScale.CompMul(-m_vPosition));

  return xiiTransformTemplate<Type>(invPos, invRot, invScale);
}
