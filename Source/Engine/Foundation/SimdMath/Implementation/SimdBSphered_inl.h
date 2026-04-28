/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdBSphered::xiiSimdBSphered() = default;

XII_ALWAYS_INLINE xiiSimdBSphered::xiiSimdBSphered(const xiiSimdVec4d& vCenter, const xiiSimdDouble& fRadius) :
  m_CenterAndRadius(vCenter)
{
  m_CenterAndRadius.SetW(fRadius);
}

XII_ALWAYS_INLINE xiiSimdBSphered xiiSimdBSphered::MakeZero()
{
  xiiSimdBSphered res;
  res.m_CenterAndRadius = xiiSimdVec4d::MakeZero();
  return res;
}

XII_ALWAYS_INLINE xiiSimdBSphered xiiSimdBSphered::MakeInvalid(const xiiSimdVec4d& vCenter /*= xiiSimdVec4d::MakeZero()*/)
{
  xiiSimdBSphered res;
  res.m_CenterAndRadius = vCenter;
  res.m_CenterAndRadius.SetW(-xiiMath::SmallEpsilon<double>());
  return res;
}

XII_ALWAYS_INLINE xiiSimdBSphered xiiSimdBSphered::MakeFromCenterAndRadius(const xiiSimdVec4d& vCenter, const xiiSimdDouble& fRadius)
{
  return xiiSimdBSphered(vCenter, fRadius);
}

inline xiiSimdBSphered xiiSimdBSphered::MakeFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiSimdVec4d)*/)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4d), "The data must not overlap.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  xiiSimdBSphered res;

  const xiiSimdVec4d* pCur = pPoints;

  xiiSimdVec4d vCenter = xiiSimdVec4d::MakeZero();
  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius = vCenter / xiiSimdDouble(uiNumPoints);

  pCur = pPoints;

  xiiSimdDouble fMaxDistSquare = xiiSimdDouble::MakeZero();
  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const xiiSimdDouble fDistSQR = (*pCur - res.m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare               = fMaxDistSquare.Max(fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt());

  return res;
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= xiiSimdDouble::MakeZero();
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>();
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdBSphered::GetCenter() const
{
  return m_CenterAndRadius;
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdBSphered::GetRadius() const
{
  return m_CenterAndRadius.w();
}

XII_ALWAYS_INLINE void xiiSimdBSphered::ExpandToInclude(const xiiSimdVec4d& vPoint)
{
  const xiiSimdDouble fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void xiiSimdBSphered::ExpandToInclude(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4d), "The data must not overlap.");

  const xiiSimdVec4d* pCur = pPoints;

  xiiSimdDouble fMaxDistSquare = xiiSimdDouble::MakeZero();

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const xiiSimdDouble fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare               = fMaxDistSquare.Max(fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

XII_ALWAYS_INLINE void xiiSimdBSphered::ExpandToInclude(const xiiSimdBSphered& rhs)
{
  const xiiSimdDouble fReqRadius = (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius();

  m_CenterAndRadius.SetW(fReqRadius.Max(GetRadius()));
}

inline void xiiSimdBSphered::Transform(const xiiSimdTransformd& t)
{
  xiiSimdVec4d newCenterAndRadius = t.TransformPosition(m_CenterAndRadius);
  newCenterAndRadius.SetW(t.GetMaxScale() * GetRadius());

  m_CenterAndRadius = newCenterAndRadius;
}

inline void xiiSimdBSphered::Transform(const xiiSimdMat4d& mMat)
{
  xiiSimdDouble radius = m_CenterAndRadius.w();
  m_CenterAndRadius    = mMat.TransformPosition(m_CenterAndRadius);

  xiiSimdDouble maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius               = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius               = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdBSphered::GetDistanceTo(const xiiSimdVec4d& vPoint) const
{
  return (vPoint - m_CenterAndRadius).GetLength<3>() - GetRadius();
}

XII_ALWAYS_INLINE xiiSimdDouble xiiSimdBSphered::GetDistanceTo(const xiiSimdBSphered& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() - GetRadius() - rhs.GetRadius();
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::Contains(const xiiSimdVec4d& vPoint) const
{
  xiiSimdDouble radius = GetRadius();
  return (vPoint - m_CenterAndRadius).GetLengthSquared<3>() <= (radius * radius);
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::Contains(const xiiSimdBSphered& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius() <= GetRadius();
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::Overlaps(const xiiSimdBSphered& rhs) const
{
  xiiSimdDouble radius = (rhs.m_CenterAndRadius + m_CenterAndRadius).w();
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLengthSquared<3>() < (radius * radius);
}

inline xiiSimdVec4d xiiSimdBSphered::GetClampedPoint(const xiiSimdVec4d& vPoint)
{
  xiiSimdVec4d  vDir  = vPoint - m_CenterAndRadius;
  xiiSimdDouble fDist = vDir.GetLengthAndNormalize<3>().Min(GetRadius());

  return m_CenterAndRadius + (vDir * fDist);
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::operator==(const xiiSimdBSphered& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet();
}

XII_ALWAYS_INLINE bool xiiSimdBSphered::operator!=(const xiiSimdBSphered& rhs) const
{
  return (m_CenterAndRadius != rhs.m_CenterAndRadius).AnySet();
}
