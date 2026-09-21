/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// This class provides functions to do atomic operations.
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
  /// Helper variable templates for easier usage
  /// Reads the atomic value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The current value.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Read(const T& ref_value);

  /// Atomically exchanges the value and returns the old one.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param newValue  - The new value to store.
  ///
  /// \return The old value before the exchange.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Exchange(T& ref_value, T newValue);

  /// Atomically increments the value and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The new value after incrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Increment(T& ref_value);

  /// Atomically decrements the value and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The new value after decrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Decrement(T& ref_value);

  /// Atomically increments the value and returns the original value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The original value before incrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T PostIncrement(T& ref_value);

  /// Atomically decrements the value and returns the original value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The original value before decrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T PostDecrement(T& ref_value);

  /// Atomically adds a value to the reference and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param addend    - Value to add.
  ///
  /// \return The new value after addition.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Add(T& ref_value, T addend);

  /// Atomically subtracts a value from the reference and returns the new value.
  ///
  /// \tparam T         - Type of the atomic value.
  /// \param ref_value  - Reference to the atomic value.
  /// \param subtrahend - Value to subtract.
  ///
  /// \return The new value after subtraction.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Subtract(T& ref_value, T subtrahend);

  /// Performs an atomic bitwise AND operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to AND with.
  ///
  /// \return The result of the AND operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T And(T& ref_value, T operand);

  /// Performs an atomic bitwise OR operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to OR with.
  ///
  /// \return The result of the OR operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Or(T& ref_value, T operand);

  /// Performs an atomic bitwise XOR operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to XOR with.
  ///
  /// \return The result of the XOR operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Xor(T& ref_value, T operand);

  /// Performs an atomic compare-and-exchange operation.
  ///
  /// \tparam T           - Type of the atomic value.
  /// \param ref_value    - Reference to the atomic value.
  /// \param ref_expected - Reference to the expected value.
  /// \param desired      - Value to set if comparison succeeds.
  ///
  /// \return True if the exchange was performed; false otherwise.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T CompareExchange(T& ref_value, T expected, T desired);

  // Performs an atomic compare-and-exchange operation.
  ///
  /// \param pDestination - Pointer to the atomic value.
  /// \param pExpected    - Pointer to the expected value.
  /// \param pValue       - Pointer to the value to set if comparison succeeds.
  ///
  /// \return True if the exchange was performed; false otherwise.
  static bool CompareExchangePointer(void** pDestination, void* pExpected, void* pValue);
};

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
#  error "Atomics are not implemented on current platform"
#endif
