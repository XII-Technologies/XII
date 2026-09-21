/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Threading/AtomicUtils.h>

/// Integer class that can be manipulated in an atomic (i.e. thread-safe) fashion.
template <typename T>
  requires xii_is_atomic_compatible_v<T>
class xiiAtomicInteger
{
public:
  XII_DECLARE_POD_TYPE();

  /// Initializes the value to zero.
  xiiAtomicInteger(); // [tested]

  /// Initializes the object with a value.
  xiiAtomicInteger(const T value); // [tested]

  /// Copy-constructor.
  xiiAtomicInteger(const xiiAtomicInteger<T>& value); // [tested]

  /// Assigns a new integer value to this object.
  xiiAtomicInteger& operator=(T value); // [tested]

  /// Assignment operator.
  xiiAtomicInteger& operator=(const xiiAtomicInteger& value); // [tested]

  /// Increments the internal value and returns the incremented value.
  T Increment(); // [tested]

  /// Decrements the internal value and returns the decremented value.
  T Decrement(); // [tested]

  /// Increments the internal value and returns the value immediately before the increment.
  T PostIncrement(); // [tested]

  /// Decrements the internal value and returns the value immediately before the decrement.
  T PostDecrement(); // [tested]

  void Add(T x);      // [tested]
  void Subtract(T x); // [tested]

  void And(T x); // [tested]
  void Or(T x);  // [tested]
  void Xor(T x); // [tested]

  void Min(T x); // [tested]
  void Max(T x); // [tested]

  /// Sets the internal value to x and returns the original internal value.
  T Set(T x); // [tested]

  /// Sets the internal value to x if the internal value is equal to expected and returns true, otherwise does nothing and returns false.
  bool TestAndSet(T expected, T x); // [tested]

  /// If this is equal to *expected*, it is set to *value*. Otherwise it won't be modified. Always returns the previous value of this before the modification.
  T CompareAndSwap(T expected, T x); // [tested]

  operator T() const; // [tested]

private:
  xii_atomic_underlying_t<T> m_Value;
};

/// An atomic boolean variable. This is just a wrapper around an atomic int32 for convenience.
class xiiAtomicBool
{
public:
  /// Initializes the bool to 'false'.
  xiiAtomicBool(); // [tested]
  ~xiiAtomicBool();

  /// Initializes the object with a value.
  xiiAtomicBool(bool value); // [tested]

  /// Copy-constructor.
  xiiAtomicBool(const xiiAtomicBool& rhs);

  /// Sets the bool to the given value and returns its previous value.
  bool Set(bool value); // [tested]

  /// Sets the bool to the given value.
  void operator=(bool value); // [tested]

  /// Sets the bool to the given value.
  void operator=(const xiiAtomicBool& rhs);

  /// Returns the current value.
  operator bool() const; // [tested]

  /// Sets the internal value to \a newValue if the internal value is equal to \a expected and returns true, otherwise does nothing and returns false.
  bool TestAndSet(bool bExpected, bool bNewValue);

private:
  xiiAtomicInteger<xiiInt32> m_iAtomicInt;
};

#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

using xiiAtomicInteger8   = xiiAtomicInteger<xiiInt8>;   // [tested]
using xiiAtomicIntegerU8  = xiiAtomicInteger<xiiUInt8>;  // [tested]
using xiiAtomicInteger16  = xiiAtomicInteger<xiiInt16>;  // [tested]
using xiiAtomicIntegerU16 = xiiAtomicInteger<xiiUInt16>; // [tested]
using xiiAtomicInteger32  = xiiAtomicInteger<xiiInt32>;  // [tested]
using xiiAtomicIntegerU32 = xiiAtomicInteger<xiiUInt32>; // [tested]
using xiiAtomicInteger64  = xiiAtomicInteger<xiiInt64>;  // [tested]
using xiiAtomicIntegerU64 = xiiAtomicInteger<xiiUInt64>; // [tested]
