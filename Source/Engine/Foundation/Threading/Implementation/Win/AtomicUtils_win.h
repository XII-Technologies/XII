#ifdef XII_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included more than once."
#endif

#define XII_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Read(const T& ref_value)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(const_cast<T*>(&ref_value)), 0, 0));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedCompareExchange16(reinterpret_cast<volatile short*>(const_cast<T*>(&ref_value)), 0, 0));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedCompareExchange(reinterpret_cast<volatile long*>(const_cast<T*>(&ref_value)), 0, 0));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(const_cast<T*>(&ref_value)), 0, 0));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Exchange(T& ref_value, T newValue)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchange8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(newValue)));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchange16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(newValue)));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchange(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(newValue)));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchange64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(newValue)));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Increment(T& ref_value)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), 1) + 1);
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), 1) + 1);
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), 1) + 1);
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), 1) + 1);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Decrement(T& ref_value)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), -1) - 1);
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), -1) - 1);
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -1) - 1);
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -1) - 1);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostIncrement(T& ref_value)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), 1));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), 1));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), 1));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), 1));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostDecrement(T& ref_value)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), -1));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), -1));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -1));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -1));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Add(T& ref_value, T addend)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(addend)) + addend);
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(addend)) + addend);
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(addend)) + addend);
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(addend)) + addend);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Subtract(T& ref_value, T subtrahend)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&ref_value), -static_cast<char>(subtrahend)) - subtrahend);
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&ref_value), -static_cast<short>(subtrahend)) - subtrahend);
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -static_cast<long>(subtrahend)) - subtrahend);
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -static_cast<__int64>(subtrahend)) - subtrahend);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::And(T& ref_value, T operand)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedAnd8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedAnd16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedAnd(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedAnd64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(operand)));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Or(T& ref_value, T operand)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedOr8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedOr16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedOr(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedOr64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(operand)));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Xor(T& ref_value, T operand)
{
  using AtomicType = xii_atomic_underlying_t<T>;

  if constexpr (sizeof(AtomicType) == 1)
  {
    return static_cast<T>(_InterlockedXor8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    return static_cast<T>(_InterlockedXor16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    return static_cast<T>(_InterlockedXor(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    return static_cast<T>(_InterlockedXor64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(operand)));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE bool xiiAtomicUtils::CompareExchange(T& ref_value, T& ref_expected, T desired)
{
  using AtomicType    = xii_atomic_underlying_t<T>;

  AtomicType original = 0;
  if constexpr (sizeof(AtomicType) == 1)
  {
    original = _InterlockedCompareExchange8(reinterpret_cast<volatile char*>(&ref_value), static_cast<char>(desired), static_cast<char>(ref_expected));
  }
  else if constexpr (sizeof(AtomicType) == 2)
  {
    original = _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(&ref_value), static_cast<short>(desired), static_cast<short>(ref_expected));
  }
  else if constexpr (sizeof(AtomicType) == 4)
  {
    original = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(desired), static_cast<long>(ref_expected));
  }
  else if constexpr (sizeof(AtomicType) == 8)
  {
    original = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(desired), static_cast<__int64>(ref_expected));
  }

  // If the original value matches ref_expected then the exchange succeeded.
  bool bSuccess = (original == static_cast<AtomicType>(ref_expected));

  // Update ref_expected with the observed value.
  ref_expected = static_cast<T>(original);

  return bSuccess;
}
