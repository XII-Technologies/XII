#ifdef XII_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Read(volatile const xiiInt32& iSrc)
{
  return _InterlockedOr((volatile long*)(&iSrc), 0);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Read(volatile const xiiInt64& iSrc)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = src;
  } while (_InterlockedCompareExchange64(const_cast<volatile xiiInt64*>(&src), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<volatile xiiInt64*>(&iSrc), 0);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Increment(volatile xiiInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Increment(volatile xiiInt64& ref_iDest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&ref_iDest);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Decrement(volatile xiiInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Decrement(volatile xiiInt64& ref_iDest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&ref_iDest);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostIncrement(volatile xiiInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostIncrement(volatile xiiInt64& ref_iDest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostDecrement(volatile xiiInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), -1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostDecrement(volatile xiiInt64& ref_iDest)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#endif
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt64& ref_iDest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedAnd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt64& ref_iDest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedOr(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt64& ref_iDest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  _InterlockedXor(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt64& ref_iDest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void xiiAtomicUtils::Min(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = ref_iDest;
    xiiInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Min(volatile xiiInt64& ref_iDest, xiiInt64 value)
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

inline void xiiAtomicUtils::Max(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = ref_iDest;
    xiiInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void xiiAtomicUtils::Max(volatile xiiInt64& ref_iDest, xiiInt64 value)
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


inline xiiInt32 xiiAtomicUtils::Set(volatile xiiInt32& ref_iDest, xiiInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Set(volatile xiiInt64& ref_iDest, xiiInt64 value)
{
#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&ref_iDest, value);
#endif
}


XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt32& ref_iDest, xiiInt32 iExpected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected) == iExpected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt64& ref_iDest, xiiInt64 iExpected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(void** volatile pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::CompareAndSwap(volatile xiiInt32& ref_iDest, xiiInt32 iExpected, xiiInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::CompareAndSwap(volatile xiiInt64& ref_iDest, xiiInt64 iExpected, xiiInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}
