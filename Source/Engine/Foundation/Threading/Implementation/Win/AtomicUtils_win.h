#ifdef XII_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included more than once."
#endif

#define XII_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Read(const T& ref_value)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedCompareExchange(reinterpret_cast<volatile long*>(const_cast<T*>(&ref_value)), 0, 0));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(const_cast<T*>(&ref_value)), 0, 0));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Exchange(T& ref_value, T newValue)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchange(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(newValue)));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchange64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(newValue)));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Increment(T& ref_value)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), 1) + 1);
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), 1) + 1);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Decrement(T& ref_value)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -1) - 1);
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -1) - 1);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostIncrement(T& ref_value)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), 1));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), 1));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::PostDecrement(T& ref_value)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -1));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -1));
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Add(T& ref_value, T addend)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    // _InterlockedExchangeAdd returns the old value.
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(addend)) + addend);
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(addend)) + addend);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Subtract(T& ref_value, T subtrahend)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_value), -static_cast<long>(subtrahend)) - subtrahend);
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&ref_value), -static_cast<__int64>(subtrahend)) - subtrahend);
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::And(T& ref_value, T operand)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedAnd(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    // For 64-bit integers, implement a compare-and-swap (CAS) loop.
    T current, desired;
    do
    {
      current = xiiAtomicUtils::AtomicRead(ref_value);
      desired = current & operand;
    } while (!xiiAtomicUtils::AtomicCompareExchange(ref_value, &current, desired));
    return current;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Or(T& ref_value, T operand)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedOr(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    T current, desired;
    do
    {
      current = xiiAtomicUtils::AtomicRead(ref_value);
      desired = current | operand;
    } while (!xiiAtomicUtils::AtomicCompareExchange(ref_value, &current, desired));
    return current;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE T xiiAtomicUtils::Xor(T& ref_value, T operand)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    return static_cast<T>(_InterlockedXor(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(operand)));
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    T current, desired;
    do
    {
      current = xiiAtomicUtils::AtomicRead(ref_value);
      desired = current ^ operand;
    } while (!xiiAtomicUtils::AtomicCompareExchange(ref_value, &current, desired));
    return current;
  }
}

template <typename T>
  requires xii_is_atomic_compatible_v<T>
XII_ALWAYS_INLINE bool xiiAtomicUtils::CompareExchange(T& ref_value, T& ref_expected, T desired)
{
  if constexpr (xiiAtomicIs32BitInteger<T>::value)
  {
    long original = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_value), static_cast<long>(desired), static_cast<long>(ref_expected));
    bool bSuccess = (original == ref_expected);
    ref_expected  = static_cast<T>(original);
    return bSuccess;
  }
  else if constexpr (xiiAtomicIs64BitInteger<T>::value)
  {
    __int64 original = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(&ref_value), static_cast<__int64>(desired), static_cast<__int64>(ref_expected));
    bool    bSuccess = (original == ref_expected);
    ref_expected     = static_cast<T>(original);
    return bSuccess;
  }
}

#if 0
XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Read(const xiiInt32& iSrc)
{
  return _InterlockedOr((long*)(&iSrc), 0);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Read(const xiiInt64& iSrc)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = iSrc;
  } while (_InterlockedCompareExchange64(const_cast<xiiInt64*>(&iSrc), old, old) != old);
  return old;
#  else
  return _InterlockedOr64(const_cast<xiiInt64*>(&iSrc), 0);
#  endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Increment(xiiInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<long*>(&ref_iDest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Increment(xiiInt64& ref_iDest)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old + 1;
#  else
  return _InterlockedIncrement64(&ref_iDest);
#  endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Decrement(xiiInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<long*>(&ref_iDest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Decrement(xiiInt64& ref_iDest)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old - 1;
#  else
  return _InterlockedDecrement64(&ref_iDest);
#  endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostIncrement(xiiInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostIncrement(xiiInt64& ref_iDest)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old;
#  else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#  endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostDecrement(xiiInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), -1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostDecrement(xiiInt64& ref_iDest)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old;
#  else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#  endif
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(xiiInt64& ref_iDest, xiiInt64 value)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + value, old) != old);
#  else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#  endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::And(xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedAnd(reinterpret_cast<long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::And(xiiInt64& ref_iDest, xiiInt64 value)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old & value, old) != old);
#  else
  _InterlockedAnd64(&ref_iDest, value);
#  endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Or(xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedOr(reinterpret_cast<long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Or(xiiInt64& ref_iDest, xiiInt64 value)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old | value, old) != old);
#  else
  _InterlockedOr64(&ref_iDest, value);
#  endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedXor(reinterpret_cast<long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(xiiInt64& ref_iDest, xiiInt64 value)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old ^ value, old) != old);
#  else
  _InterlockedXor64(&ref_iDest, value);
#  endif
}


inline void xiiAtomicUtils::Min(xiiInt32& ref_iDest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = ref_iDest;
    xiiInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Min(xiiInt64& ref_iDest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = ref_iDest;
    xiiInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Max(xiiInt32& ref_iDest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = ref_iDest;
    xiiInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Max(xiiInt64& ref_iDest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = ref_iDest;
    xiiInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline xiiInt32 xiiAtomicUtils::Set(xiiInt32& ref_iDest, xiiInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Set(xiiInt64& ref_iDest, xiiInt64 value)
{
#  if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, value, old) != old);
  return old;
#  else
  return _InterlockedExchange64(&ref_iDest, value);
#  endif
}


XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(xiiInt32& ref_iDest, xiiInt32 iExpected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected) == iExpected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(xiiInt64& ref_iDest, xiiInt64 iExpected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(void** pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::CompareAndSwap(xiiInt32& ref_iDest, xiiInt32 iExpected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::CompareAndSwap(xiiInt64& ref_iDest, xiiInt64 iExpected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}
#endif
