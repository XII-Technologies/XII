
template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger() :
  m_value(0)
{
}

template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger(T value) :
  m_value(value)
{
}

template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger(const xiiAtomicInteger<T>& value) :
  m_value(value.m_value)
{
}

template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>& xiiAtomicInteger<T>::operator=(const T value)
{
  m_value = value;
  return *this;
}

template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>& xiiAtomicInteger<T>::operator=(const xiiAtomicInteger<T>& value)
{
  m_value = value.m_value;
  return *this;
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Increment()
{
  return xiiAtomicUtils::Increment(m_value);
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Decrement()
{
  return xiiAtomicUtils::Decrement(m_value);
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostIncrement()
{
  return xiiAtomicUtils::PostIncrement(m_value);
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostDecrement()
{
  return xiiAtomicUtils::PostDecrement(m_value);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Add(T x)
{
  xiiAtomicUtils::Add(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Subtract(T x)
{
  xiiAtomicUtils::Add(m_value, -x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::And(T x)
{
  xiiAtomicUtils::And(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Or(T x)
{
  xiiAtomicUtils::Or(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Xor(T x)
{
  xiiAtomicUtils::Xor(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Min(T x)
{
  xiiAtomicUtils::Min(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Max(T x)
{
  xiiAtomicUtils::Max(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Set(T x)
{
  return xiiAtomicUtils::Set(m_value, x);
}

template <typename T>
XII_ALWAYS_INLINE bool xiiAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return xiiAtomicUtils::TestAndSet(m_value, expected, x);
}

template <typename T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return xiiAtomicUtils::CompareAndSwap(m_value, expected, x);
}

template <typename T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::operator T() const
{
  return xiiAtomicUtils::Read(m_value);
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiAtomicBool::xiiAtomicBool()  = default;
XII_ALWAYS_INLINE xiiAtomicBool::~xiiAtomicBool() = default;

XII_ALWAYS_INLINE xiiAtomicBool::xiiAtomicBool(bool value)
{
  Set(value);
}

XII_ALWAYS_INLINE xiiAtomicBool::xiiAtomicBool(const xiiAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

XII_ALWAYS_INLINE bool xiiAtomicBool::Set(bool value)
{
  return m_iAtomicInt.Set(value ? 1 : 0) != 0;
}

XII_ALWAYS_INLINE void xiiAtomicBool::operator=(bool value)
{
  Set(value);
}

XII_ALWAYS_INLINE void xiiAtomicBool::operator=(const xiiAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

XII_ALWAYS_INLINE xiiAtomicBool::operator bool() const
{
  return static_cast<xiiInt32>(m_iAtomicInt) != 0;
}

XII_ALWAYS_INLINE bool xiiAtomicBool::TestAndSet(bool expected, bool newValue)
{
  return m_iAtomicInt.TestAndSet(expected ? 1 : 0, newValue ? 1 : 0) != 0;
}
