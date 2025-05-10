
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger() :
  m_Value(0)
{
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger(T value) :
  m_Value(value)
{
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger(const xiiAtomicInteger<T>& value) :
  m_Value(xiiAtomicUtils::Read(value.m_Value))
{
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>& xiiAtomicInteger<T>::operator=(const T value)
{
  Set(value);
  return *this;
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>& xiiAtomicInteger<T>::operator=(const xiiAtomicInteger<T>& value)
{
  Set(xiiAtomicUtils::Read(value.m_Value));
  return *this;
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Increment()
{
  return xiiAtomicUtils::Increment(m_Value);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Decrement()
{
  return xiiAtomicUtils::Decrement(m_Value);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostIncrement()
{
  return xiiAtomicUtils::PostIncrement(m_Value);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostDecrement()
{
  return xiiAtomicUtils::PostDecrement(m_Value);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Add(T x)
{
  m_Value = xiiAtomicUtils::Add(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Subtract(T x)
{
  m_Value = xiiAtomicUtils::Subtract(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::And(T x)
{
  m_Value = xiiAtomicUtils::And(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Or(T x)
{
  m_Value = xiiAtomicUtils::Or(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Xor(T x)
{
  m_Value = xiiAtomicUtils::Xor(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Min(T x)
{
  T current = xiiAtomicUtils::Read(m_Value);
  while (current > x)
  {
    T expected = current;
    if (xiiAtomicUtils::CompareExchange(m_Value, expected, x))
      break;
    current = expected;
  }
  xiiAtomicUtils::Min(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Max(T x)
{
  T current = xiiAtomicUtils::Read(m_Value);
  while (current < x)
  {
    T expected = current;
    if (xiiAtomicUtils::CompareExchange(m_Value, expected, x))
      break;
    current = expected;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Set(T x)
{
  return xiiAtomicUtils::Exchange(m_Value, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE bool xiiAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return xiiAtomicUtils::CompareExchange(m_Value, expected, x);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return xiiAtomicUtils::CompareExchange(m_Value, expected, x);
  return expected;
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::operator T() const
{
  return xiiAtomicUtils::Read(m_Value);
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

XII_ALWAYS_INLINE bool xiiAtomicBool::TestAndSet(bool bExpected, bool bNewValue)
{
  return m_iAtomicInt.TestAndSet(bExpected ? 1 : 0, bNewValue ? 1 : 0) != 0;
}
