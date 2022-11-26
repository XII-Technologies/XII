#ifdef XII_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Read(volatile const xiiInt32& src)
{
  return _InterlockedOr((volatile long*)(&src), 0);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Read(volatile const xiiInt64& src)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = src;
  } while (_InterlockedCompareExchange64(const_cast<volatile xiiInt64*>(&src), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<volatile xiiInt64*>(&src), 0);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Increment(volatile xiiInt32& dest)
{
  return _InterlockedIncrement(reinterpret_cast<volatile long*>(&dest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Increment(volatile xiiInt64& dest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&dest);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Decrement(volatile xiiInt32& dest)
{
  return _InterlockedDecrement(reinterpret_cast<volatile long*>(&dest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Decrement(volatile xiiInt64& dest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&dest);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostIncrement(volatile xiiInt32& dest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&dest), 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostIncrement(volatile xiiInt64& dest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&dest, 1);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostDecrement(volatile xiiInt32& dest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&dest), -1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostDecrement(volatile xiiInt64& dest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&dest, -1);
#endif
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt32& dest, xiiInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&dest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt64& dest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&dest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt32& dest, xiiInt32 value)
{
  _InterlockedAnd(reinterpret_cast<volatile long*>(&dest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt64& dest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old & value, old) != old);
#else
  _InterlockedAnd64(&dest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt32& dest, xiiInt32 value)
{
  _InterlockedOr(reinterpret_cast<volatile long*>(&dest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt64& dest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old | value, old) != old);
#else
  _InterlockedOr64(&dest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt32& dest, xiiInt32 value)
{
  _InterlockedXor(reinterpret_cast<volatile long*>(&dest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt64& dest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old ^ value, old) != old);
#else
  _InterlockedXor64(&dest, value);
#endif
}


inline void xiiAtomicUtils::Min(volatile xiiInt32& dest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = dest;
    xiiInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&dest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Min(volatile xiiInt64& dest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = dest;
    xiiInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&dest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Max(volatile xiiInt32& dest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = dest;
    xiiInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&dest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Max(volatile xiiInt64& dest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = dest;
    xiiInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&dest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline xiiInt32 xiiAtomicUtils::Set(volatile xiiInt32& dest, xiiInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<volatile long*>(&dest), value);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Set(volatile xiiInt64& dest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&dest, value);
#endif
}


XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&dest), value, expected) == expected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&dest, value, expected) == expected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(void** volatile dest, void* expected, void* value)
{
  return _InterlockedCompareExchangePointer(dest, value, expected) == expected;
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::CompareAndSwap(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&dest), value, expected);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::CompareAndSwap(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&dest, value, expected);
}
