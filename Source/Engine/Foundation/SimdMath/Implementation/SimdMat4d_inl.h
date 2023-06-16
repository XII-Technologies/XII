#pragma once

XII_ALWAYS_INLINE xiiSimdMat4d::xiiSimdMat4d() = default;

XII_ALWAYS_INLINE xiiSimdMat4d::xiiSimdMat4d(const double* const pData, xiiMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

XII_ALWAYS_INLINE xiiSimdMat4d::xiiSimdMat4d(const xiiSimdVec4d& vCol0, const xiiSimdVec4d& vCol1, const xiiSimdVec4d& vCol2, const xiiSimdVec4d& vCol3)
{
  m_col0 = vCol0;
  m_col1 = vCol1;
  m_col2 = vCol2;
  m_col3 = vCol3;
}

XII_ALWAYS_INLINE xiiSimdMat4d::xiiSimdMat4d(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3, double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4)
{
  m_col0.Set(f1r1, f1r2, f1r3, f1r4);
  m_col1.Set(f2r1, f2r2, f2r3, f2r4);
  m_col2.Set(f3r1, f3r2, f3r3, f3r4);
  m_col3.Set(f4r1, f4r2, f4r3, f4r4);
}

inline void xiiSimdMat4d::SetFromArray(const double* const pData, xiiMatrixLayout::Enum layout)
{
  m_col0.Load<4>(pData + 0);
  m_col1.Load<4>(pData + 4);
  m_col2.Load<4>(pData + 8);
  m_col3.Load<4>(pData + 12);

  if (layout == xiiMatrixLayout::RowMajor)
  {
    Transpose();
  }
}

inline void xiiSimdMat4d::GetAsArray(double* out_pData, xiiMatrixLayout::Enum layout) const
{
  xiiSimdMat4d tmp = *this;

  if (layout == xiiMatrixLayout::RowMajor)
  {
    tmp.Transpose();
  }

  tmp.m_col0.Store<4>(out_pData + 0);
  tmp.m_col1.Store<4>(out_pData + 4);
  tmp.m_col2.Store<4>(out_pData + 8);
  tmp.m_col3.Store<4>(out_pData + 12);
}

XII_ALWAYS_INLINE void xiiSimdMat4d::SetIdentity()
{
  m_col0.Set(1, 0, 0, 0);
  m_col1.Set(0, 1, 0, 0);
  m_col2.Set(0, 0, 1, 0);
  m_col3.Set(0, 0, 0, 1);
}

// static
XII_ALWAYS_INLINE xiiSimdMat4d xiiSimdMat4d::IdentityMatrix()
{
  xiiSimdMat4d result;
  result.SetIdentity();
  return result;
}

XII_ALWAYS_INLINE xiiSimdMat4d xiiSimdMat4d::GetTranspose() const
{
  xiiSimdMat4d result = *this;
  result.Transpose();
  return result;
}

XII_ALWAYS_INLINE xiiSimdMat4d xiiSimdMat4d::GetInverse(const xiiSimdDouble& fEpsilon) const
{
  xiiSimdMat4d result = *this;
  result.Invert(fEpsilon).IgnoreResult();
  return result;
}

inline bool xiiSimdMat4d::IsEqual(const xiiSimdMat4d& rhs, const xiiSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(rhs.m_col0, fEpsilon) && m_col1.IsEqual(rhs.m_col1, fEpsilon) && m_col2.IsEqual(rhs.m_col2, fEpsilon) &&
          m_col3.IsEqual(rhs.m_col3, fEpsilon))
    .AllSet<4>();
}

inline bool xiiSimdMat4d::IsIdentity(const xiiSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(xiiSimdVec4d(1, 0, 0, 0), fEpsilon) && m_col1.IsEqual(xiiSimdVec4d(0, 1, 0, 0), fEpsilon) &&
          m_col2.IsEqual(xiiSimdVec4d(0, 0, 1, 0), fEpsilon) && m_col3.IsEqual(xiiSimdVec4d(0, 0, 0, 1), fEpsilon))
    .AllSet<4>();
}

inline bool xiiSimdMat4d::IsValid() const
{
  return m_col0.IsValid<4>() && m_col1.IsValid<4>() && m_col2.IsValid<4>() && m_col3.IsValid<4>();
}

inline bool xiiSimdMat4d::IsNaN() const
{
  return m_col0.IsNaN<4>() || m_col1.IsNaN<4>() || m_col2.IsNaN<4>() || m_col3.IsNaN<4>();
}

XII_ALWAYS_INLINE void xiiSimdMat4d::SetRows(const xiiSimdVec4d& vRow0, const xiiSimdVec4d& vRow1, const xiiSimdVec4d& vRow2, const xiiSimdVec4d& vRow3)
{
  m_col0 = vRow0;
  m_col1 = vRow1;
  m_col2 = vRow2;
  m_col3 = vRow3;

  Transpose();
}

XII_ALWAYS_INLINE void xiiSimdMat4d::GetRows(xiiSimdVec4d& ref_vRow0, xiiSimdVec4d& ref_vRow1, xiiSimdVec4d& ref_vRow2, xiiSimdVec4d& ref_vRow3) const
{
  xiiSimdMat4d tmp = *this;
  tmp.Transpose();

  ref_vRow0 = tmp.m_col0;
  ref_vRow1 = tmp.m_col1;
  ref_vRow2 = tmp.m_col2;
  ref_vRow3 = tmp.m_col3;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdMat4d::TransformPosition(const xiiSimdVec4d& v) const
{
  xiiSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();
  result += m_col3;

  return result;
}

XII_ALWAYS_INLINE xiiSimdVec4d xiiSimdMat4d::TransformDirection(const xiiSimdVec4d& v) const
{
  xiiSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();

  return result;
}

XII_ALWAYS_INLINE xiiSimdMat4d xiiSimdMat4d::operator*(const xiiSimdMat4d& rhs) const
{
  xiiSimdMat4d result;

  result.m_col0 = m_col0 * rhs.m_col0.x();
  result.m_col0 += m_col1 * rhs.m_col0.y();
  result.m_col0 += m_col2 * rhs.m_col0.z();
  result.m_col0 += m_col3 * rhs.m_col0.w();

  result.m_col1 = m_col0 * rhs.m_col1.x();
  result.m_col1 += m_col1 * rhs.m_col1.y();
  result.m_col1 += m_col2 * rhs.m_col1.z();
  result.m_col1 += m_col3 * rhs.m_col1.w();

  result.m_col2 = m_col0 * rhs.m_col2.x();
  result.m_col2 += m_col1 * rhs.m_col2.y();
  result.m_col2 += m_col2 * rhs.m_col2.z();
  result.m_col2 += m_col3 * rhs.m_col2.w();

  result.m_col3 = m_col0 * rhs.m_col3.x();
  result.m_col3 += m_col1 * rhs.m_col3.y();
  result.m_col3 += m_col2 * rhs.m_col3.z();
  result.m_col3 += m_col3 * rhs.m_col3.w();

  return result;
}

XII_ALWAYS_INLINE void xiiSimdMat4d::operator*=(const xiiSimdMat4d& rhs)
{
  *this = *this * rhs;
}

XII_ALWAYS_INLINE bool xiiSimdMat4d::operator==(const xiiSimdMat4d& other) const
{
  return (m_col0 == other.m_col0 && m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3).AllSet<4>();
}

XII_ALWAYS_INLINE bool xiiSimdMat4d::operator!=(const xiiSimdMat4d& other) const
{
  return !(*this == other);
}
