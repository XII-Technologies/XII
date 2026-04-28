/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#ifdef XII_ATOMICUTLS_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_ATOMICUTLS_POSIX_INL_H_INCLUDED

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Read(const T& ref_value)
{
  return __atomic_load_n(&ref_value, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Exchange(T& ref_value, T newValue)
{
  return __atomic_exchange_n(&ref_value, newValue, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Increment(T& ref_value)
{
  return __atomic_add_fetch(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Decrement(T& ref_value)
{
  return __atomic_sub_fetch(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostIncrement(T& ref_value)
{
  return __atomic_fetch_add(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostDecrement(T& ref_value)
{
  return __atomic_fetch_sub(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Add(T& ref_value, T addend)
{
  return __atomic_add_fetch(&ref_value, addend, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Subtract(T& ref_value, T subtrahend)
{
  return __atomic_sub_fetch(&ref_value, subtrahend, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::And(T& ref_value, T operand)
{
  return __atomic_and_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Or(T& ref_value, T operand)
{
  return __atomic_or_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Xor(T& ref_value, T operand)
{
  return __atomic_xor_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::CompareExchange(T& ref_value, T expected, T desired)
{
  T original = expected;
  __atomic_compare_exchange_n(&ref_value, &original, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return original;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::CompareExchangePointer(void** pDestination, void* pExpected, void* pValue)
{
  void* expected = pExpected;
  return __atomic_compare_exchange_n(pDestination, &expected, pValue, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
