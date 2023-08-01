#ifdef XII_ATOMICUTLS_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_ATOMICUTLS_POSIX_INL_H_INCLUDED


#include <Foundation/Math/Math.h>

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Read(volatile const xiiInt32& src)
{
  return __sync_fetch_and_or(const_cast<volatile xiiInt32*>(&src), 0);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Read(volatile const xiiInt64& src)
{
  return __sync_fetch_and_or_8(const_cast<volatile xiiInt64*>(&src), 0);
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Increment(volatile xiiInt32& dest)
{
  return __sync_add_and_fetch(&dest, 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Increment(volatile xiiInt64& dest)
{
  return __sync_add_and_fetch_8(&dest, 1);
}


XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Decrement(volatile xiiInt32& dest)
{
  return __sync_sub_and_fetch(&dest, 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Decrement(volatile xiiInt64& dest)
{
  return __sync_sub_and_fetch_8(&dest, 1);
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostIncrement(volatile xiiInt32& dest)
{
  return __sync_fetch_and_add(&dest, 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostIncrement(volatile xiiInt64& dest)
{
  return __sync_fetch_and_add_8(&dest, 1);
}


XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::PostDecrement(volatile xiiInt32& dest)
{
  return __sync_fetch_and_sub(&dest, 1);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::PostDecrement(volatile xiiInt64& dest)
{
  return __sync_fetch_and_sub_8(&dest, 1);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt32& dest, xiiInt32 value)
{
  __sync_fetch_and_add(&dest, value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Add(volatile xiiInt64& dest, xiiInt64 value)
{
  __sync_fetch_and_add_8(&dest, value);
}


XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt32& dest, xiiInt32 value)
{
  __sync_fetch_and_and(&dest, value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::And(volatile xiiInt64& dest, xiiInt64 value)
{
  __sync_fetch_and_and_8(&dest, value);
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt32& dest, xiiInt32 value)
{
  __sync_fetch_and_or(&dest, value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Or(volatile xiiInt64& dest, xiiInt64 value)
{
  __sync_fetch_and_or_8(&dest, value);
}


XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt32& dest, xiiInt32 value)
{
  __sync_fetch_and_xor(&dest, value);
}

XII_ALWAYS_INLINE void xiiAtomicUtils::Xor(volatile xiiInt64& dest, xiiInt64 value)
{
  __sync_fetch_and_xor_8(&dest, value);
}


XII_FORCE_INLINE void xiiAtomicUtils::Min(volatile xiiInt32& dest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = dest;
    xiiInt32 iNewValue = xiiMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

XII_FORCE_INLINE void xiiAtomicUtils::Min(volatile xiiInt64& dest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = dest;
    xiiInt64 iNewValue = xiiMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


XII_FORCE_INLINE void xiiAtomicUtils::Max(volatile xiiInt32& dest, xiiInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt32 iOldValue = dest;
    xiiInt32 iNewValue = xiiMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

XII_FORCE_INLINE void xiiAtomicUtils::Max(volatile xiiInt64& dest, xiiInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    xiiInt64 iOldValue = dest;
    xiiInt64 iNewValue = xiiMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::Set(volatile xiiInt32& dest, xiiInt32 value)
{
  return __sync_lock_test_and_set(&dest, value);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::Set(volatile xiiInt64& dest, xiiInt64 value)
{
  return __sync_lock_test_and_set_8(&dest, value);
}


XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value)
{
  return __sync_bool_compare_and_swap(&dest, expected, value);
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value)
{
  return __sync_bool_compare_and_swap_8(&dest, expected, value);
}

XII_ALWAYS_INLINE bool xiiAtomicUtils::TestAndSet(void** dest, void* expected, void* value)
{
#if XII_ENABLED(XII_PLATFORM_64BIT)
  xiiUInt64* puiTemp = reinterpret_cast<xiiUInt64*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<xiiUInt64>(expected), reinterpret_cast<xiiUInt64>(value));
#else
  xiiUInt32* puiTemp = reinterpret_cast<xiiUInt32*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<xiiUInt32>(expected), reinterpret_cast<xiiUInt32>(value));
#endif
}

XII_ALWAYS_INLINE xiiInt32 xiiAtomicUtils::CompareAndSwap(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value)
{
  return __sync_val_compare_and_swap(&dest, expected, value);
}

XII_ALWAYS_INLINE xiiInt64 xiiAtomicUtils::CompareAndSwap(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value)
{
  return __sync_val_compare_and_swap_8(&dest, expected, value);
}
