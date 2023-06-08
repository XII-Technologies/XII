
XII_ALWAYS_INLINE xiiRational::xiiRational() = default;

XII_ALWAYS_INLINE xiiRational::xiiRational(xiiUInt32 uiNumerator, xiiUInt32 uiDenominator) :
  m_uiNumerator(uiNumerator), m_uiDenominator(uiDenominator)
{
}

XII_ALWAYS_INLINE bool xiiRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

XII_ALWAYS_INLINE bool xiiRational::operator==(const xiiRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

XII_ALWAYS_INLINE bool xiiRational::operator!=(const xiiRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRational::GetNumerator() const
{
  return m_uiNumerator;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRational::GetDenominator() const
{
  return m_uiDenominator;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

XII_ALWAYS_INLINE double xiiRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);
}

XII_ALWAYS_INLINE bool xiiRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

XII_ALWAYS_INLINE xiiRational xiiRational::ReduceIntegralFraction() const
{
  XII_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return xiiRational(m_uiNumerator / m_uiDenominator, 1);
}
