#pragma once

XII_ALWAYS_INLINE xiiSimdTransform::xiiSimdTransform() = default;

XII_ALWAYS_INLINE xiiSimdTransform::xiiSimdTransform(const xiiSimdVec4f& vPosition, const xiiSimdQuat& qRotation, const xiiSimdVec4f& vScale) :
  m_Position(vPosition),
  m_Rotation(qRotation),
  m_Scale(vScale)
{
}

XII_ALWAYS_INLINE xiiSimdTransform::xiiSimdTransform(const xiiSimdQuat& qRotation) :
  m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

XII_ALWAYS_INLINE void xiiSimdTransform::SetIdentity()
{
  m_Position.SetZero();
  m_Rotation.SetIdentity();
  m_Scale.Set(1.0f);
}

// static
XII_ALWAYS_INLINE xiiSimdTransform xiiSimdTransform::IdentityTransform()
{
  xiiSimdTransform result;
  result.SetIdentity();
  return result;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdTransform::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

XII_ALWAYS_INLINE bool xiiSimdTransform::ContainsNegativeScale() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < xiiSimdFloat::Zero();
}

XII_ALWAYS_INLINE bool xiiSimdTransform::ContainsUniformScale() const
{
  const xiiSimdFloat fEpsilon = xiiMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

XII_ALWAYS_INLINE bool xiiSimdTransform::IsEqual(const xiiSimdTransform& rhs, const xiiSimdFloat& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
    m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

XII_ALWAYS_INLINE void xiiSimdTransform::Invert()
{
  (*this) = GetInverse();
}

XII_ALWAYS_INLINE xiiSimdTransform xiiSimdTransform::GetInverse() const
{
  xiiSimdQuat  invRot   = -m_Rotation;
  xiiSimdVec4f invScale = m_Scale.GetReciprocal();
  xiiSimdVec4f invPos   = invRot * (invScale.CompMul(-m_Position));

  return xiiSimdTransform(invPos, invRot, invScale);
}

inline void xiiSimdTransform::SetLocalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& globalTransformChild)
{
  xiiSimdQuat  invRot   = -globalTransformParent.m_Rotation;
  xiiSimdVec4f invScale = globalTransformParent.m_Scale.GetReciprocal();

  m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  m_Rotation = invRot * globalTransformChild.m_Rotation;
  m_Scale    = invScale.CompMul(globalTransformChild.m_Scale);
}

XII_ALWAYS_INLINE void xiiSimdTransform::SetGlobalTransform(const xiiSimdTransform& globalTransformParent, const xiiSimdTransform& localTransformChild)
{
  *this = globalTransformParent * localTransformChild;
}

XII_FORCE_INLINE xiiSimdMat4f xiiSimdTransform::GetAsMat4() const
{
  xiiSimdMat4f result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdTransform::TransformPosition(const xiiSimdVec4f& v) const
{
  const xiiSimdVec4f scaled  = m_Scale.CompMul(v);
  const xiiSimdVec4f rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdTransform::TransformDirection(const xiiSimdVec4f& v) const
{
  const xiiSimdVec4f scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

XII_ALWAYS_INLINE const xiiSimdVec4f operator*(const xiiSimdTransform& t, const xiiSimdVec4f& v)
{
  return t.TransformPosition(v);
}

inline const xiiSimdTransform operator*(const xiiSimdTransform& lhs, const xiiSimdTransform& rhs)
{
  xiiSimdTransform t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransform::operator*=(const xiiSimdTransform& other)
{
  (*this) = (*this) * other;
}

XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdTransform& lhs, const xiiSimdQuat& q)
{
  xiiSimdTransform t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale    = lhs.m_Scale;
  return t;
}

XII_ALWAYS_INLINE const xiiSimdTransform operator*(const xiiSimdQuat& q, const xiiSimdTransform& rhs)
{
  xiiSimdTransform t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale    = rhs.m_Scale;
  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransform::operator*=(const xiiSimdQuat& q)
{
  m_Rotation = m_Rotation * q;
}

XII_ALWAYS_INLINE const xiiSimdTransform operator+(const xiiSimdTransform& lhs, const xiiSimdVec4f& v)
{
  xiiSimdTransform t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale;

  return t;
}

XII_ALWAYS_INLINE const xiiSimdTransform operator-(const xiiSimdTransform& lhs, const xiiSimdVec4f& v)
{
  xiiSimdTransform t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale    = lhs.m_Scale;

  return t;
}

XII_ALWAYS_INLINE void xiiSimdTransform::operator+=(const xiiSimdVec4f& v)
{
  m_Position += v;
}

XII_ALWAYS_INLINE void xiiSimdTransform::operator-=(const xiiSimdVec4f& v)
{
  m_Position -= v;
}

XII_ALWAYS_INLINE bool operator==(const xiiSimdTransform& lhs, const xiiSimdTransform& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

XII_ALWAYS_INLINE bool operator!=(const xiiSimdTransform& lhs, const xiiSimdTransform& rhs)
{
  return !(lhs == rhs);
}
