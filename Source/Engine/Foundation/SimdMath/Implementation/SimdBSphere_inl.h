#pragma once

XII_ALWAYS_INLINE xiiSimdBSphere::xiiSimdBSphere() = default;

XII_ALWAYS_INLINE xiiSimdBSphere::xiiSimdBSphere(const xiiSimdVec4f& vCenter, const xiiSimdFloat& fRadius) :
  m_CenterAndRadius(vCenter)
{
  m_CenterAndRadius.SetW(fRadius);
}

XII_ALWAYS_INLINE xiiSimdBSphere xiiSimdBSphere::MakeZero()
{
  xiiSimdBSphere res;
  res.m_CenterAndRadius = xiiSimdVec4f::MakeZero();
  return res;
}

XII_ALWAYS_INLINE xiiSimdBSphere xiiSimdBSphere::MakeInvalid(const xiiSimdVec4f& vCenter /*= xiiSimdVec4f::MakeZero()*/)
{
  xiiSimdBSphere res;
  res.m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -xiiMath::SmallEpsilon<float>());
  return res;
}

XII_ALWAYS_INLINE xiiSimdBSphere xiiSimdBSphere::MakeFromCenterAndRadius(const xiiSimdVec4f& vCenter, const xiiSimdFloat& fRadius)
{
  return xiiSimdBSphere(vCenter, fRadius);
}

inline xiiSimdBSphere xiiSimdBSphere::MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiSimdVec4f)*/)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4f), "The data must not overlap.");
  XII_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  xiiSimdBSphere res;

  const xiiSimdVec4f* pCur = pPoints;

  xiiSimdVec4f vCenter = xiiSimdVec4f::MakeZero();
  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius = vCenter / xiiSimdFloat(uiNumPoints);

  pCur = pPoints;

  xiiSimdFloat fMaxDistSquare = xiiSimdFloat::MakeZero();
  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const xiiSimdFloat fDistSQR = (*pCur - res.m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare              = fMaxDistSquare.Max(fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt());

  return res;
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= xiiSimdFloat::MakeZero();
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>();
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdBSphere::GetCenter() const
{
  return m_CenterAndRadius;
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdBSphere::GetRadius() const
{
  return m_CenterAndRadius.w();
}

XII_ALWAYS_INLINE void xiiSimdBSphere::ExpandToInclude(const xiiSimdVec4f& vPoint)
{
  const xiiSimdFloat fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void xiiSimdBSphere::ExpandToInclude(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4f), "The data must not overlap.");

  const xiiSimdVec4f* pCur = pPoints;

  xiiSimdFloat fMaxDistSquare = xiiSimdFloat::MakeZero();

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const xiiSimdFloat fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare              = fMaxDistSquare.Max(fDistSQR);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

XII_ALWAYS_INLINE void xiiSimdBSphere::ExpandToInclude(const xiiSimdBSphere& rhs)
{
  const xiiSimdFloat fReqRadius = (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius();

  m_CenterAndRadius.SetW(fReqRadius.Max(GetRadius()));
}

inline void xiiSimdBSphere::Transform(const xiiSimdTransform& t)
{
  xiiSimdVec4f newCenterAndRadius = t.TransformPosition(m_CenterAndRadius);
  newCenterAndRadius.SetW(t.GetMaxScale() * GetRadius());

  m_CenterAndRadius = newCenterAndRadius;
}

inline void xiiSimdBSphere::Transform(const xiiSimdMat4f& mMat)
{
  xiiSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius   = mMat.TransformPosition(m_CenterAndRadius);

  xiiSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius              = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius              = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdBSphere::GetDistanceTo(const xiiSimdVec4f& vPoint) const
{
  return (vPoint - m_CenterAndRadius).GetLength<3>() - GetRadius();
}

XII_ALWAYS_INLINE xiiSimdFloat xiiSimdBSphere::GetDistanceTo(const xiiSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() - GetRadius() - rhs.GetRadius();
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::Contains(const xiiSimdVec4f& vPoint) const
{
  xiiSimdFloat radius = GetRadius();
  return (vPoint - m_CenterAndRadius).GetLengthSquared<3>() <= (radius * radius);
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::Contains(const xiiSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius() <= GetRadius();
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::Overlaps(const xiiSimdBSphere& rhs) const
{
  xiiSimdFloat radius = (rhs.m_CenterAndRadius + m_CenterAndRadius).w();
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLengthSquared<3>() < (radius * radius);
}

inline xiiSimdVec4f xiiSimdBSphere::GetClampedPoint(const xiiSimdVec4f& vPoint)
{
  xiiSimdVec4f vDir  = vPoint - m_CenterAndRadius;
  xiiSimdFloat fDist = vDir.GetLengthAndNormalize<3>().Min(GetRadius());

  return m_CenterAndRadius + (vDir * fDist);
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::operator==(const xiiSimdBSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet();
}

XII_ALWAYS_INLINE bool xiiSimdBSphere::operator!=(const xiiSimdBSphere& rhs) const
{
  return (m_CenterAndRadius != rhs.m_CenterAndRadius).AnySet();
}
