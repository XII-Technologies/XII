#pragma once

#include <Foundation/Basics.h>

/// \brief Helper trait to determine if an enum has a 32-bit underlying type and provide the underlying type.
template <typename T, typename = void>
struct xiiIsEnumWith32BitUnderlyingType : std::false_type
{
  using UnderlyingType = void; // Default to void for non-enum types.
};

template <typename T>
struct xiiIsEnumWith32BitUnderlyingType<T, std::enable_if_t<std::is_enum_v<T> && (sizeof(std::underlying_type_t<T>) <= 4U)>> : std::true_type
{
  using UnderlyingType = std::underlying_type_t<T>;
};

/// \brief Helper trait to determine if an enum has a 64-bit underlying type and provide the underlying type.
template <typename T, typename = void>
struct xiiIsEnumWith64BitUnderlyingType : std::false_type
{
  using UnderlyingType = void; // Default to void for non-enum types.
};

template <typename T>
struct xiiIsEnumWith64BitUnderlyingType<T, std::enable_if_t<std::is_enum_v<T> && (sizeof(std::underlying_type_t<T>) >= 8U)>> : std::true_type
{
  using UnderlyingType = std::underlying_type_t<T>;
};

/// \brief Custom trait to determine if a type is a 32-bit integer or has a 32-bit underlying type.
template <typename T>
struct xiiAtomicIs32BitInteger
{
private:
  static constexpr bool IsEnumWith32BitUnderlyingType = xiiIsEnumWith32BitUnderlyingType<T>::value;

public:
  static constexpr bool value =
    std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || IsEnumWith32BitUnderlyingType;

  using UnderlyingType = std::conditional_t<
    IsEnumWith32BitUnderlyingType,
    typename xiiIsEnumWith32BitUnderlyingType<T>::UnderlyingType,
    T>;
};

/// \brief Custom trait to determine if a type is a 64-bit integer or has a 64-bit underlying type.
template <typename T>
struct xiiAtomicIs64BitInteger
{
private:
  static constexpr bool IsEnumWith64BitUnderlyingType = xiiIsEnumWith64BitUnderlyingType<T>::value;

public:
  static constexpr bool value =
    std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> || IsEnumWith64BitUnderlyingType;

  using UnderlyingType = std::conditional_t<
    IsEnumWith64BitUnderlyingType,
    typename xiiIsEnumWith64BitUnderlyingType<T>::UnderlyingType,
    T>;
};

/// \brief General trait to check atomic compatibility (only 32-bit or 64-bit integers).
template <typename T>
struct xiiAtomicCompatible
{
private:
  static constexpr bool Is32Bit = xiiAtomicIs32BitInteger<T>::value;
  static constexpr bool Is64Bit = xiiAtomicIs64BitInteger<T>::value;

public:
  static constexpr bool value = Is32Bit || Is64Bit;

  using UnderlyingType = std::conditional_t<
    Is32Bit, typename xiiAtomicIs32BitInteger<T>::UnderlyingType,
    std::conditional_t<Is64Bit, typename xiiAtomicIs64BitInteger<T>::UnderlyingType, void>>;
};

template <typename T> constexpr bool xii_is_atomic_compatible_v = xiiAtomicCompatible<T>::value;
template <typename T> using xii_atomic_underlying_t             = typename xiiAtomicCompatible<T>::UnderlyingType;



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
  /// Helper variable templates for easier usage
  /// \brief Reads the atomic value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The current value.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Read(const T& ref_value);

  /// \brief Atomically exchanges the value and returns the old one.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param newValue  - The new value to store.
  ///
  /// \return The old value before the exchange.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Exchange(T& ref_value, T newValue);

  /// \brief Atomically increments the value and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The new value after incrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Increment(T& ref_value);

  /// \brief Atomically decrements the value and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The new value after decrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Decrement(T& ref_value);

  /// \brief Atomically increments the value and returns the original value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The original value before incrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T PostIncrement(T& ref_value);

  /// \brief Atomically decrements the value and returns the original value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  ///
  /// \return The original value before decrementing.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T PostDecrement(T& ref_value);

  /// \brief Atomically adds a value to the reference and returns the new value.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param addend    - Value to add.
  ///
  /// \return The new value after addition.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Add(T& ref_value, T addend);

  /// \brief Atomically subtracts a value from the reference and returns the new value.
  ///
  /// \tparam T         - Type of the atomic value.
  /// \param ref_value  - Reference to the atomic value.
  /// \param subtrahend - Value to subtract.
  ///
  /// \return The new value after subtraction.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Subtract(T& ref_value, T subtrahend);

  /// \brief Performs an atomic bitwise AND operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to AND with.
  ///
  /// \return The result of the AND operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T And(T& ref_value, T operand);

  /// \brief Performs an atomic bitwise OR operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to OR with.
  ///
  /// \return The result of the OR operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Or(T& ref_value, T operand);

  /// \brief Performs an atomic bitwise XOR operation.
  ///
  /// \tparam T        - Type of the atomic value.
  /// \param ref_value - Reference to the atomic value.
  /// \param operand   - Value to XOR with.
  ///
  /// \return The result of the XOR operation.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static T Xor(T& ref_value, T operand);

  /// \brief Performs an atomic compare-and-exchange operation.
  ///
  /// \tparam T           - Type of the atomic value.
  /// \param ref_value    - Reference to the atomic value.
  /// \param ref_expected - Reference to the expected value.
  /// \param desired      - Value to set if comparison succeeds.
  ///
  /// \return True if the exchange was performed; false otherwise.
  template <typename T>
    requires xii_is_atomic_compatible_v<T>
  static bool CompareExchange(T& ref_value, T& ref_expected, T desired);
};

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
#  error "Atomics are not implemented on current platform"
#endif
