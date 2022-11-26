#pragma once

#include <Foundation/Basics.h>

/// \brief This class provides functions to do atomic operations.
///
/// Atomic operations are generally faster than mutexes, and should therefore be preferred whenever possible.
/// However only the operations in themselves are atomic, once you execute several of them in sequence,
/// the sequence will not be atomic.
/// Also atomic operations are a lot slower than non-atomic operations, thus you should not use them in code that
/// does not need to be thread-safe.
/// xiiAtomicInteger is built on top of xiiAtomicUtils and provides a more convenient interface to use atomic
/// integer instructions.
struct XII_FOUNDATION_DLL xiiAtomicUtils
{
  /// \brief Returns src as an atomic operation and returns its value.
  static xiiInt32 Read(volatile const xiiInt32& src); // [tested]

  /// \brief Returns src as an atomic operation and returns its value.
  static xiiInt64 Read(volatile const xiiInt64& src); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static xiiInt32 Increment(volatile xiiInt32& dest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static xiiInt64 Increment(volatile xiiInt64& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static xiiInt32 Decrement(volatile xiiInt32& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static xiiInt64 Decrement(volatile xiiInt64& dest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static xiiInt32 PostIncrement(volatile xiiInt32& dest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static xiiInt64 PostIncrement(volatile xiiInt64& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static xiiInt32 PostDecrement(volatile xiiInt32& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static xiiInt64 PostDecrement(volatile xiiInt64& dest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static xiiInt32 Set(volatile xiiInt32& dest, xiiInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static xiiInt64 Set(volatile xiiInt64& dest, xiiInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** volatile dest, void* expected, void* value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static xiiInt32 CompareAndSwap(volatile xiiInt32& dest, xiiInt32 expected, xiiInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static xiiInt64 CompareAndSwap(volatile xiiInt64& dest, xiiInt64 expected, xiiInt64 value); // [tested]
};

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
#  error "Atomics are not implemented on current platform"
#endif
