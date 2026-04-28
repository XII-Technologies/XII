/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

XII_ALWAYS_INLINE xiiSimdBBox::xiiSimdBBox() = default;

XII_ALWAYS_INLINE xiiSimdBBox::xiiSimdBBox(const xiiSimdVec4f& vMin, const xiiSimdVec4f& vMax) :
  m_Min(vMin), m_Max(vMax)
{
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBox::MakeZero()
{
  return xiiSimdBBox(xiiSimdVec4f::MakeZero(), xiiSimdVec4f::MakeZero());
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBox::MakeInvalid()
{
  return xiiSimdBBox(xiiSimdVec4f(xiiMath::MaxValue<float>()), xiiSimdVec4f(-xiiMath::MaxValue<float>()));
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBox::MakeFromCenterAndHalfExtents(const xiiSimdVec4f& vCenter, const xiiSimdVec4f& vHalfExtents)
{
  return xiiSimdBBox(vCenter - vHalfExtents, vCenter + vHalfExtents);
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBox::MakeFromMinMax(const xiiSimdVec4f& vMin, const xiiSimdVec4f& vMax)
{
  return xiiSimdBBox(vMin, vMax);
}

XII_ALWAYS_INLINE xiiSimdBBox xiiSimdBBox::MakeFromPoints(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride /*= sizeof(xiiSimdVec4f)*/)
{
  xiiSimdBBox box = xiiSimdBBox::MakeInvalid();
  box.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return box;
}

XII_ALWAYS_INLINE bool xiiSimdBBox::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBox::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdBBox::GetCenter() const
{
  return (m_Min + m_Max) * xiiSimdFloat(0.5f);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdBBox::GetExtents() const
{
  return m_Max - m_Min;
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdBBox::GetHalfExtents() const
{
  return (m_Max - m_Min) * xiiSimdFloat(0.5f);
}

XII_ALWAYS_INLINE void xiiSimdBBox::ExpandToInclude(const xiiSimdVec4f& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void xiiSimdBBox::ExpandToInclude(const xiiSimdVec4f* pPoints, xiiUInt32 uiNumPoints, xiiUInt32 uiStride)
{
  XII_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  XII_ASSERT_DEBUG(uiStride >= sizeof(xiiSimdVec4f), "Data may not overlap.");

  const xiiSimdVec4f* pCur = pPoints;

  for (xiiUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = xiiMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

XII_ALWAYS_INLINE void xiiSimdBBox::ExpandToInclude(const xiiSimdBBox& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void xiiSimdBBox::ExpandToCube()
{
  const xiiSimdVec4f center      = GetCenter();
  const xiiSimdVec4f halfExtents = center - m_Min;

  *this = xiiSimdBBox::MakeFromCenterAndHalfExtents(center, xiiSimdVec4f(halfExtents.HorizontalMax<3>()));
}

XII_ALWAYS_INLINE bool xiiSimdBBox::Contains(const xiiSimdVec4f& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBox::Contains(const xiiSimdBBox& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool xiiSimdBBox::Contains(const xiiSimdBSphere& rhs) const
{
  const xiiSimdBBox otherBox = xiiSimdBBox::MakeFromCenterAndHalfExtents(rhs.GetCenter(), xiiSimdVec4f(rhs.GetRadius()));

  return Contains(otherBox);
}

XII_ALWAYS_INLINE bool xiiSimdBBox::Overlaps(const xiiSimdBBox& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool xiiSimdBBox::Overlaps(const xiiSimdBSphere& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

XII_ALWAYS_INLINE void xiiSimdBBox::Grow(const xiiSimdVec4f& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

XII_ALWAYS_INLINE void xiiSimdBBox::Translate(const xiiSimdVec4f& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

XII_ALWAYS_INLINE void xiiSimdBBox::Transform(const xiiSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

XII_ALWAYS_INLINE void xiiSimdBBox::Transform(const xiiSimdMat4f& mMat)
{
  const xiiSimdVec4f center      = GetCenter();
  const xiiSimdVec4f halfExtents = center - m_Min;

  const xiiSimdVec4f newCenter = mMat.TransformPosition(center);

  xiiSimdVec4f newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  *this = xiiSimdBBox::MakeFromCenterAndHalfExtents(newCenter, newHalfExtents);
}

XII_ALWAYS_INLINE xiiSimdVec4f xiiSimdBBox::GetClampedPoint(const xiiSimdVec4f& vPoint) const
{
  return vPoint.CompMin(m_Max).CompMax(m_Min);
}

inline xiiSimdFloat xiiSimdBBox::GetDistanceSquaredTo(const xiiSimdVec4f& vPoint) const
{
  const xiiSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared<3>();
}

inline xiiSimdFloat xiiSimdBBox::GetDistanceTo(const xiiSimdVec4f& vPoint) const
{
  const xiiSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBox::operator==(const xiiSimdBBox& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

XII_ALWAYS_INLINE bool xiiSimdBBox::operator!=(const xiiSimdBBox& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}
