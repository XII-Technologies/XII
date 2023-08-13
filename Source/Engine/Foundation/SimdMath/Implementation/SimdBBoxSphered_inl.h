#pragma once

XII_ALWAYS_INLINE xiiSimdBBoxSphered::xiiSimdBBoxSphered() = default;

XII_ALWAYS_INLINE xiiSimdBBoxSphered::xiiSimdBBoxSphered(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vBoxHalfExtents, const xiiSimdDouble& fSphereRadius) :
  m_CenterAndRadius(vCenter), m_BoxHalfExtents(vBoxHalfExtents)
{
  m_CenterAndRadius.SetW(fSphereRadius);
}

inline xiiSimdBBoxSphered::xiiSimdBBoxSphered(const xiiSimdBBoxd& box, const xiiSimdBSphered& sphere) :
  m_CenterAndRadius(box.GetCenter()), m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>().Min((sphere.GetCenter() - m_CenterAndRadius).GetLength<3>() + sphere.GetRadius()));
}

inline xiiSimdBBoxSphered::xiiSimdBBoxSphered(const xiiSimdBBoxd& box) :
  m_CenterAndRadius(box.GetCenter()), m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>());
}

XII_ALWAYS_INLINE xiiSimdBBoxSphered::xiiSimdBBoxSphered(const xiiSimdBSphered& sphere) :
  m_CenterAndRadius(sphere.m_CenterAndRadius), m_BoxHalfExtents(xiiSimdVec4d(sphere.GetRadius()))
{
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphered::SetInvalid()
{
  m_CenterAndRadius.Set(0.0, 0.0, 0.0, -xiiMath::SmallEpsilon<double>());
  m_BoxHalfExtents.Set(-xiiMath::MaxValue<double>());
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphered::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && m_CenterAndRadius.w() >= xiiSimdDouble::Zero() && m_BoxHalfExtents.IsValid<3>() && (m_BoxHalfExtents >= xiiSimdVec4d::ZeroVector()).AllSet<3>();
}

inline bool xiiSimdBBoxSphered::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>() || m_BoxHalfExtents.IsNaN<3>();
}

inline void xiiSimdBBoxSphered::SetFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  xiiSimdBBoxd box;
  box.SetFromPoints(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents  = m_CenterAndRadius - box.m_Min;

  xiiSimdBSphered sphere(m_CenterAndRadius, xiiSimdDouble::Zero());
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius.SetW(sphere.GetRadius());
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxSphered::GetBox() const
{
  xiiSimdBBoxd box;
  box.SetCenterAndHalfExtents(m_CenterAndRadius, m_BoxHalfExtents);
  return box;
}

XII_ALWAYS_INLINE xiiSimdBSphered xiiSimdBBoxSphered::GetSphere() const
{
  xiiSimdBSphered sphere;
  sphere.m_CenterAndRadius = m_CenterAndRadius;
  return sphere;
}

inline void xiiSimdBBoxSphered::ExpandToInclude(const xiiSimdBBoxSphered& rhs)
{
  xiiSimdBBoxd box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  xiiSimdVec4d  center         = box.GetCenter();
  xiiSimdVec4d  boxHalfExtents = center - box.m_Min;
  xiiSimdDouble tmpRadius      = boxHalfExtents.GetLength<3>();

  const xiiSimdDouble fSphereRadiusA = (m_CenterAndRadius - center).GetLength<3>() + m_CenterAndRadius.w();
  const xiiSimdDouble fSphereRadiusB = (rhs.m_CenterAndRadius - center).GetLength<3>() + rhs.m_CenterAndRadius.w();

  m_CenterAndRadius = center;
  m_CenterAndRadius.SetW(tmpRadius.Min(fSphereRadiusA.Max(fSphereRadiusB)));
  m_BoxHalfExtents = boxHalfExtents;
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphered::Transform(const xiiSimdTransformd& t)
{
  Transform(t.GetAsMat4());
}

XII_ALWAYS_INLINE void xiiSimdBBoxSphered::Transform(const xiiSimdMat4d& mMat)
{
  xiiSimdDouble radius = m_CenterAndRadius.w();
  m_CenterAndRadius    = mMat.TransformPosition(m_CenterAndRadius);

  xiiSimdDouble maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius               = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius               = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);

  xiiSimdVec4d newHalfExtents = mMat.m_col0.Abs() * m_BoxHalfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * m_BoxHalfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * m_BoxHalfExtents.z();

  m_BoxHalfExtents = newHalfExtents.CompMin(xiiSimdVec4d(radius));
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphered::operator==(const xiiSimdBBoxSphered& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet<4>() && (m_BoxHalfExtents == rhs.m_BoxHalfExtents).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxSphered::operator!=(const xiiSimdBBoxSphered& rhs) const
{
  return !(*this == rhs);
}
