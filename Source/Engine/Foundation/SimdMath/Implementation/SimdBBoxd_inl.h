#pragma once

XII_ALWAYS_INLINE xiiSimdBBoxd::xiiSimdBBoxd() = default;

XII_ALWAYS_INLINE xiiSimdBBoxd::xiiSimdBBoxd(const xiiSimdVec4d& vMin, const xiiSimdVec4d& vMax) :
  m_Min(vMin), m_Max(vMax)
{
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxd::MakeZero()
{
  return xiiSimdBBoxd(xiiSimdVec4d::MakeZero(), xiiSimdVec4d::MakeZero());
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxd::MakeInvalid()
{
  return xiiSimdBBoxd(xiiSimdVec4d(xiiMath::MaxValue<float>()), xiiSimdVec4d(-xiiMath::MaxValue<float>()));
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxd::MakeFromCenterAndHalfExtents(const xiiSimdVec4d& vCenter, const xiiSimdVec4d& vHalfExtents)
{
  return xiiSimdBBoxd(vCenter - vHalfExtents, vCenter + vHalfExtents);
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxd::MakeFromMinMax(const xiiSimdVec4d& vMin, const xiiSimdVec4d& vMax)
{
  return xiiSimdBBoxd(vMin, vMax);
}

XII_ALWAYS_INLINE xiiSimdBBoxd xiiSimdBBoxd::MakeFromPoints(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiSimdVec4d)*/)
{
  xiiSimdBBoxd box = xiiSimdBBoxd::MakeInvalid();
  box.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return box;
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdBBoxd::GetCenter() const
{
  return (m_Min + m_Max) * xiiSimdDouble(0.5);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdBBoxd::GetExtents() const
{
  return m_Max - m_Min;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdBBoxd::GetHalfExtents() const
{
  return (m_Max - m_Min) * xiiSimdDouble(0.5);
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::ExpandToInclude(const xiiSimdVec4d& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void xiiSimdBBoxd::ExpandToInclude(const xiiSimdVec4d* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4d), "Data may not overlap.");

  const xiiSimdVec4d* pCur = pPoints;

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::ExpandToInclude(const xiiSimdBBoxd& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void xiiSimdBBoxd::ExpandToCube()
{
  const xiiSimdVec4d center      = GetCenter();
  const xiiSimdVec4d halfExtents = center - m_Min;

  *this = xiiSimdBBoxd::MakeFromCenterAndHalfExtents(center, xiiSimdVec4d(halfExtents.HorizontalMax<3>()));
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::Contains(const xiiSimdVec4d& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::Contains(const xiiSimdBBoxd& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool xiiSimdBBoxd::Contains(const xiiSimdBSphered& rhs) const
{
  const xiiSimdBBoxd otherBox = xiiSimdBBoxd::MakeFromCenterAndHalfExtents(rhs.GetCenter(), xiiSimdVec4d(rhs.GetRadius()));

  return Contains(otherBox);
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::Overlaps(const xiiSimdBBoxd& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool xiiSimdBBoxd::Overlaps(const xiiSimdBSphered& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::Grow(const xiiSimdVec4d& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::Translate(const xiiSimdVec4d& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::Transform(const xiiSimdTransformd& t)
{
  Transform(t.GetAsMat4());
}

XII_ALWAYS_INLINE void xiiSimdBBoxd::Transform(const xiiSimdMat4d& mMat)
{
  const xiiSimdVec4d center      = GetCenter();
  const xiiSimdVec4d halfExtents = center - m_Min;

  const xiiSimdVec4d newCenter = mMat.TransformPosition(center);

  xiiSimdVec4d newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  *this = xiiSimdBBoxd::MakeFromCenterAndHalfExtents(newCenter, newHalfExtents);
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdBBoxd::GetClampedPoint(const xiiSimdVec4d& vPoint) const
{
  return vPoint.CompMin(m_Max).CompMax(m_Min);
}

inline xiiSimdDouble xiiSimdBBoxd::GetDistanceSquaredTo(const xiiSimdVec4d& vPoint) const
{
  const xiiSimdVec4d vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared<3>();
}

inline xiiSimdDouble xiiSimdBBoxd::GetDistanceTo(const xiiSimdVec4d& vPoint) const
{
  const xiiSimdVec4d vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::operator==(const xiiSimdBBoxd& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBoxd::operator!=(const xiiSimdBBoxd& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}
