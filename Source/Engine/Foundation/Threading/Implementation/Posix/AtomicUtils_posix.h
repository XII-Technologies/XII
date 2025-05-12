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

// Exchange: Atomically swaps the value with a new one, and returns the old value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Exchange(T& ref_value, T newValue)
{
  return __atomic_exchange_n(&ref_value, newValue, __ATOMIC_SEQ_CST);
}

// Increment: Atomically adds 1 to the value and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Increment(T& ref_value)
{
  return __atomic_add_fetch(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

// Decrement: Atomically subtracts 1 from the value and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Decrement(T& ref_value)
{
  return __atomic_sub_fetch(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

// PostIncrement: Atomically adds 1 but returns the previous value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostIncrement(T& ref_value)
{
  return __atomic_fetch_add(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

// PostDecrement: Atomically subtracts 1 but returns the previous value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostDecrement(T& ref_value)
{
  return __atomic_fetch_sub(&ref_value, static_cast<T>(1), __ATOMIC_SEQ_CST);
}

// Add: Atomically adds addend to the value and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Add(T& ref_value, T addend)
{
  return __atomic_add_fetch(&ref_value, addend, __ATOMIC_SEQ_CST);
}

// Subtract: Atomically subtracts subtrahend from the value and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Subtract(T& ref_value, T subtrahend)
{
  return __atomic_sub_fetch(&ref_value, subtrahend, __ATOMIC_SEQ_CST);
}

// And: Atomically performs a bitwise AND with the operand and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::And(T& ref_value, T operand)
{
  return __atomic_and_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

// Or: Atomically performs a bitwise OR with the operand and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Or(T& ref_value, T operand)
{
  return __atomic_or_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

// Xor: Atomically performs a bitwise XOR with the operand and returns the new value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Xor(T& ref_value, T operand)
{
  return __atomic_xor_fetch(&ref_value, operand, __ATOMIC_SEQ_CST);
}

// CompareExchange: Atomically compares ref_value against expected, and if equal,
// sets it to desired. Returns the original value held in ref_value.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::CompareExchange(T& ref_value, T expected, T desired)
{
  T original = expected;
  __atomic_compare_exchange_n(&ref_value,        // pointer to the memory
                              &original,         // expected value, updated if failure
                              desired,           // desired value if comparison is successful
                              false,             // do not allow spurious failures
                              __ATOMIC_SEQ_CST,  // memory order on success
                              __ATOMIC_SEQ_CST); // memory order on failure
  return original;
}

// CompareExchangePointer: Specialized function for pointer types. It
// compares the pointer *pDestination against pExpected and, if equal, atomically
// replaces it with pValue. Returns true on a successful exchange.
XII_ALWAYS_INLINE bool xiiAtomicUtils::CompareExchangePointer(void** pDestination, void* pExpected, void* pValue)
{
  void* expected = pExpected;
  return __atomic_compare_exchange_n(pDestination,
                                     &expected,
                                     pValue,
                                     false,
                                     __ATOMIC_SEQ_CST,
                                     __ATOMIC_SEQ_CST);
}
