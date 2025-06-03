
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger() :
  m_Value(0)
{
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::xiiAtomicInteger(T value) :
  m_Value(static_cast<xii_atomic_underlying_t<T>>(value))
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
  return static_cast<T>(xiiAtomicUtils::Increment(m_Value));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Decrement()
{
  return static_cast<T>(xiiAtomicUtils::Decrement(m_Value));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostIncrement()
{
  return static_cast<T>(xiiAtomicUtils::PostIncrement(m_Value));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::PostDecrement()
{
  return static_cast<T>(xiiAtomicUtils::PostDecrement(m_Value));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Add(T x)
{
  xiiAtomicUtils::Add(m_Value, static_cast<xii_atomic_underlying_t<T>>(x));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Subtract(T x)
{
  xiiAtomicUtils::Subtract(m_Value, static_cast<xii_atomic_underlying_t<T>>(x));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::And(T x)
{
  xiiAtomicUtils::And(m_Value, static_cast<xii_atomic_underlying_t<T>>(x));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Or(T x)
{
  xiiAtomicUtils::Or(m_Value, static_cast<xii_atomic_underlying_t<T>>(x));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Xor(T x)
{
  xiiAtomicUtils::Xor(m_Value, static_cast<xii_atomic_underlying_t<T>>(x));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Min(T x)
{
  T current = static_cast<T>(xiiAtomicUtils::Read(m_Value));
  while (true)
  {
    if (current <= x)
      break;

    T expected = current;
    T newValue = x;

    current = xiiAtomicUtils::CompareExchange(m_Value, static_cast<xii_atomic_underlying_t<T>>(expected), static_cast<xii_atomic_underlying_t<T>>(newValue));

    if (current == expected)
      break;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE void xiiAtomicInteger<T>::Max(T x)
{
  T current = static_cast<T>(xiiAtomicUtils::Read(m_Value));
  while (true)
  {
    if (current >= x)
      break;

    T expected = current;
    T newValue = x;

    current = xiiAtomicUtils::CompareExchange(m_Value, static_cast<xii_atomic_underlying_t<T>>(expected), static_cast<xii_atomic_underlying_t<T>>(newValue));

    if (current == expected)
      break;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::Set(T x)
{
  return static_cast<T>(xiiAtomicUtils::Exchange(m_Value, static_cast<xii_atomic_underlying_t<T>>(x)));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE bool xiiAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return static_cast<T>(xiiAtomicUtils::CompareExchange(m_Value, static_cast<xii_atomic_underlying_t<T>>(expected), static_cast<xii_atomic_underlying_t<T>>(x)) == expected);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return static_cast<T>(xiiAtomicUtils::CompareExchange(m_Value, static_cast<xii_atomic_underlying_t<T>>(expected), static_cast<xii_atomic_underlying_t<T>>(x)));
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE xiiAtomicInteger<T>::operator T() const
{
  return static_cast<T>(xiiAtomicUtils::Read(m_Value));
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
