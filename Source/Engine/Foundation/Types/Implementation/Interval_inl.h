
template <class Type>
constexpr xiiInterval<Type>::xiiInterval(Type startAndEndValue) :
  m_StartValue(startAndEndValue), m_EndValue(startAndEndValue)
{
}

template <class Type>
constexpr xiiInterval<Type>::xiiInterval(Type start, Type end) :
  m_StartValue(start), m_EndValue(xiiMath::Max(start, end))
{
}

template <class Type>
void xiiInterval<Type>::SetStartAdjustEnd(Type value)
{
  m_StartValue = value;
  m_EndValue   = xiiMath::Max(m_EndValue, m_StartValue);
}

template <class Type>
void xiiInterval<Type>::SetEndAdjustStart(Type value)
{
  m_EndValue   = value;
  m_StartValue = xiiMath::Min(m_StartValue, m_EndValue);
}

template <class Type>
void xiiInterval<Type>::ClampToIntervalAdjustEnd(Type minValue, Type maxValue, Type minimumSeparation /*= Type()*/)
{
  // clamp the start value to the valid range, leave minimumSeparation at the end
  m_StartValue = xiiMath::Clamp(m_StartValue, minValue, maxValue - minimumSeparation);

  // clamp the start value to the remaining range
  m_EndValue = xiiMath::Clamp(m_EndValue, m_StartValue, maxValue);
}

template <class Type>
void xiiInterval<Type>::ClampToIntervalAdjustStart(Type minValue, Type maxValue, Type minimumSeparation /*= Type()*/)
{
  // clamp the end value to the valid range, leave minimumSeparation at the start
  m_EndValue = xiiMath::Clamp(m_EndValue, minValue + minimumSeparation, maxValue);

  // clamp the start value to the remaining range
  m_StartValue = xiiMath::Clamp(m_StartValue, minValue, m_EndValue);
}

template <class Type>
Type xiiInterval<Type>::GetSeparation() const
{
  return m_EndValue - m_StartValue;
}

template <class Type>
bool xiiInterval<Type>::operator==(const xiiInterval<Type>& rhs) const
{
  return m_StartValue == rhs.m_StartValue && m_EndValue == rhs.m_EndValue;
}
