#pragma once

XII_ALWAYS_INLINE xiiSimdBBoxSphere::xiiSimdBBoxSphere() = default;

XII_ALWAYS_INLINE xiiSimdBBoxSphere::xiiSimdBBoxSphere(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vBoxHalfExtents, const xiiSimdFloat& fSphereRadius)
{
  m_CenterAndRadius = vCenter;
  m_CenterAndRadius.SetW(fSphereRadius);
  m_BoxHalfExtents = vBoxHalfExtents;
}

inline xiiSimdBBoxSphere::xiiSimdBBoxSphere(const xiiSimdBBox& box, const xiiSimdBSphere& sphere)
{
  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents  = m_CenterAndRadius - box.m_Min;
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>().Min((sphere.GetCenter() - m_CenterAndRadius).GetLength<3>() + sphere.GetRadius()));
}

inline xiiSimdBBoxSphere::xiiSimdBBoxSphere(const xiiSimdBBox& box)
{
  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents  = m_CenterAndRadius - box.m_Min;
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>());
}

XII_ALWAYS_INLINE xiiSimdBBoxSphere::xiiSimdBBoxSphere(const xiiSimdBSphere& sphere)
{
  m_CenterAndRadius = sphere.m_CenterAndRadius;
  m_BoxHalfExtents  = xiiSimdVec4f(sphere.GetRadius());
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -xiiMath::SmallEpsilon<float>());
  m_BoxHalfExtents.Set(-xiiMath::MaxValue<float>());
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && m_CenterAndRadius.w() >= xiiSimdFloat::Zero() && m_BoxHalfExtents.IsValid<3>() &&
    (m_BoxHalfExtents >= xiiSimdVec4f::ZeroVector()).AllSet<3>();
}

inline bool xiiSimdBBoxSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>() || m_BoxHalfExtents.IsNaN<3>();
}

inline void xiiSimdBBoxSphere::SetFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  xiiSimdBBox box;
  box.SetFromPoints(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents  = m_CenterAndRadius - box.m_Min;

  xiiSimdBSphere sphere(m_CenterAndRadius, xiiSimdFloat::Zero());
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius.SetW(sphere.GetRadius());
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBoxSphere::GetBox() const
{
  xiiSimdBBox box;
  box.SetCenterAndHalfExtents(m_CenterAndRadius, m_BoxHalfExtents);
  return box;
}

XII_ALWAYS_INLINE xiiSimdBSphere xiiSimdBBoxSphere::GetSphere() const
{
  xiiSimdBSphere sphere;
  sphere.m_CenterAndRadius = m_CenterAndRadius;
  return sphere;
}

inline void xiiSimdBBoxSphere::ExpandToInclude(const xiiSimdBBoxSphere& rhs)
{
  xiiSimdBBox box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  xiiSimdVec4f center         = box.GetCenter();
  xiiSimdVec4f boxHalfExtents = center - box.m_Min;
  xiiSimdFloat tmpRadius      = boxHalfExtents.GetLength<3>();

  const xiiSimdFloat fSphereRadiusA = (m_CenterAndRadius - center).GetLength<3>() + m_CenterAndRadius.w();
  const xiiSimdFloat fSphereRadiusB = (rhs.m_CenterAndRadius - center).GetLength<3>() + rhs.m_CenterAndRadius.w();

  m_CenterAndRadius = center;
  m_CenterAndRadius.SetW(tmpRadius.Min(fSphereRadiusA.Max(fSphereRadiusB)));
  m_BoxHalfExtents = boxHalfExtents;
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphere::Transform(const xiiSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphere::Transform(const xiiSimdMat4f& mMat)
{
  xiiSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius   = mMat.TransformPosition(m_CenterAndRadius);

  xiiSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius              = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius              = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);

  xiiSimdVec4f newHalfExtents = mMat.m_col0.Abs() * m_BoxHalfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * m_BoxHalfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * m_BoxHalfExtents.z();

  m_BoxHalfExtents = newHalfExtents.CompMin(xiiSimdVec4f(radius));
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphere::operator==(const xiiSimdBBoxSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet<4>() && (m_BoxHalfExtents == rhs.m_BoxHalfExtents).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphere::operator!=(const xiiSimdBBoxSphere& rhs) const
{
  return !(*this == rhs);
}
