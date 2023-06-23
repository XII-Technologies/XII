#pragma once

XII_ALWAYS_INLINE xiiSimdTransformd::xiiSimdTransformd() = default;

XII_ALWAYS_INLINE xiiSimdTransformd::xiiSimdTransformd(const xiiSimdVec4d& vPosition, const xiiSimdQuatd& rotation, const xiiSimdVec4d& vScale)
{
  m_Position = vPosition;
  m_Rotation = rotation;
  m_Scale    = vScale;
}

XII_ALWAYS_INLINE xiiSimdTransformd::xiiSimdTransformd(const xiiSimdQuatd& rotation)
{
  m_Position.SetZero();
  m_Rotation = rotation;
  m_Scale.Set(1.0);
}

XII_ALWAYS_INLINE void xiiSimdTransformd::SetIdentity()
{
  m_Position.SetZero();
  m_Rotation.SetIdentity();
  m_Scale.Set(1.0);
}

// static
XII_ALWAYS_INLINE xiiSimdTransformd xiiSimdTransformd::IdentityTransform()
{
  xiiSimdTransformd result;
  result.SetIdentity();
  return result;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdTransformd::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

XII_ALWAYS_INLINE bool xiiSimdTransformd::ContainsNegativeScale() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < xiiSimdDouble::Zero();
}

XII_ALWAYS_INLINE bool xiiSimdTransformd::ContainsUniformScale() const
{
  const xiiSimdDouble fEpsilon = xiiMath::DefaultEpsilon<double>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdTransformd::IsEqual(const xiiSimdTransformd& rhs, const xiiSimdDouble& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) && m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

XII_ALWAYS_INLINE void xiiSimdTransformd::Invert()
{
  (*this) = GetInverse();
}

XII_ALWAYS_INLINE xiiSimdTransformd xiiSimdTransformd::GetInverse() const
{
  xiiSimdQuatd invRot   = -m_Rotation;
  xiiSimdVec4d invScale = m_Scale.GetReciprocal();
  xiiSimdVec4d invPos   = invRot * (invScale.CompMul(-m_Position));

  return xiiSimdTransformd(invPos, invRot, invScale);
}

inline void xiiSimdTransformd::SetLocalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& globalTransformChild)
{
  xiiSimdQuatd invRot   = -globalTransformParent.m_Rotation;
  xiiSimdVec4d invScale = globalTransformParent.m_Scale.GetReciprocal();

  m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  m_Rotation = invRot * globalTransformChild.m_Rotation;
  m_Scale    = invScale.CompMul(globalTransformChild.m_Scale);
}

XII_ALWAYS_INLINE void xiiSimdTransformd::SetGlobalTransform(const xiiSimdTransformd& globalTransformParent, const xiiSimdTransformd& localTransformChild)
{
  *this = globalTransformParent * localTransformChild;
}

XII_FORCE_INLINE xiiSimdMat4d xiiSimdTransformd::GetAsMat4() const
{
  xiiSimdMat4d result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdTransformd::TransformPosition(const xiiSimdVec4d& v) const
{
  const xiiSimdVec4d scaled  = m_Scale.CompMul(v);
  const xiiSimdVec4d rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdTransformd::TransformDirection(const xiiSimdVec4d& v) const
{
  const xiiSimdVec4d scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

XII_ALWAYS_INLINE const xiiSimdVec4d operator*(const xiiSimdTransformd& t, const xiiSimdVec4d& v)
{
  return t.TransformPosition(v);
}

inline const xiiSimdTransformd operator*(const xiiSimdTransformd& lhs, const xiiSimdTransformd& rhs)
{
  xiiSimdTransformd t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransformd::operator*=(const xiiSimdTransformd& other)
{
  (*this) = (*this) * other;
}

XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdTransformd& lhs, const xiiSimdQuatd& q)
{
  xiiSimdTransformd t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale    = lhs.m_Scale;
  return t;
}

XII_ALWAYS_INLINE const xiiSimdTransformd operator*(const xiiSimdQuatd& q, const xiiSimdTransformd& rhs)
{
  xiiSimdTransformd t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale    = rhs.m_Scale;
  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransformd::operator*=(const xiiSimdQuatd& q)
{
  m_Rotation = m_Rotation * q;
}

XII_ALWAYS_INLINE const xiiSimdTransformd operator+(const xiiSimdTransformd& lhs, const xiiSimdVec4d& v)
{
  xiiSimdTransformd t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale;

  return t;
}

XII_ALWAYS_INLINE const xiiSimdTransformd operator-(const xiiSimdTransformd& lhs, const xiiSimdVec4d& v)
{
  xiiSimdTransformd t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale;

  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransformd::operator+=(const xiiSimdVec4d& v)
{
  m_Position += v;
}

XII_ALWAYS_INLINE void xiiSimdTransformd::operator-=(const xiiSimdVec4d& v)
{
  m_Position -= v;
}

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransformd& lhs, const xiiSimdTransformd& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransformd& lhs, const xiiSimdTransformd& rhs)
{
  return !(lhs == rhs);
}
