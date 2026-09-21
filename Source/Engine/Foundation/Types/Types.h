/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// ***** Definition of types *****

#include <cstdint>

using xiiUInt8  = uint8_t;
using xiiUInt16 = uint16_t;
using xiiUInt32 = uint32_t;
using xiiUInt64 = unsigned long long;

using xiiInt8  = int8_t;
using xiiInt16 = int16_t;
using xiiInt32 = int32_t;
using xiiInt64 = signed long long;

#if XII_ENABLED(XII_DOUBLE_PRECISION)
using xiiReal = double;
#else
using xiiReal = float;
#endif

// Do some compile-time checks on the types
static_assert(sizeof(bool) == 1);
static_assert(sizeof(char) == 1);
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);
static_assert(sizeof(xiiInt8) == 1);
static_assert(sizeof(xiiInt16) == 2);
static_assert(sizeof(xiiInt32) == 4);
static_assert(sizeof(xiiInt64) == 8); // Must be defined in the specific compiler header.
static_assert(sizeof(xiiUInt8) == 1);
static_assert(sizeof(xiiUInt16) == 2);
static_assert(sizeof(xiiUInt32) == 4);
static_assert(sizeof(xiiUInt64) == 8); // Must be defined in the specific compiler header.
static_assert(sizeof(long long int) == 8);

#if XII_ENABLED(XII_PLATFORM_64BIT)
#  define XII_ALIGNMENT_MINIMUM 8
#elif XII_ENABLED(XII_PLATFORM_32BIT)
#  define XII_ALIGNMENT_MINIMUM 4
#else
#  error "Unknown pointer size."
#endif

static_assert(sizeof(void*) == XII_ALIGNMENT_MINIMUM);
static_assert(alignof(void*) == XII_ALIGNMENT_MINIMUM);

/// Enum values representing success or failure states.
///
/// Typically used to return execution status from functions, as an alternative to using plain booleans.
enum xiiResultEnum : xiiUInt8
{
  XII_FAILURE = 0U, ///< Indicates that the operation failed.
  XII_SUCCESS       ///< Indicates that the operation succeeded.
};

/// Encapsulates a result state (success or failure), with [[nodiscard]] enforcement.
///
/// Provides clearer semantics than using raw booleans and encourages consistent status checking in functions and APIs.
/// Also includes convenience utilities for asserting or ignoring result values.
struct [[nodiscard]] XII_FOUNDATION_DLL xiiResult
{
public:
  /// \name Constructors and Assignments
  /// @{

  /// Constructs a result from an enum value.
  ///
  /// Example:
  /// \code
  /// xiiResult result = XII_SUCCESS;
  /// \endcode
  XII_ALWAYS_INLINE xiiResult(xiiResultEnum result) :
    m_E(result)
  {
  }

  /// Assigns a new result state.
  XII_ALWAYS_INLINE void operator=(xiiResultEnum rhs) { m_E = rhs; }

  /// Compares the result to another enum value.
  XII_ALWAYS_INLINE bool operator==(xiiResultEnum cmp) const { return m_E == cmp; }

  /// @}

  /// \name Status Queries
  /// @{

  /// Returns true if the result indicates success.
  [[nodiscard]] XII_ALWAYS_INLINE bool Succeeded() const { return m_E == XII_SUCCESS; }

  /// Returns true if the result indicates failure.
  [[nodiscard]] XII_ALWAYS_INLINE bool Failed() const { return m_E == XII_FAILURE; }

  /// @}

  /// \name Control and Assertions
  /// @{

  /// Suppresses compiler warnings when intentionally ignoring the result.
  ///
  /// To be used when result checking isn't necessary, e.g., in best-effort cleanup code.
  XII_ALWAYS_INLINE void IgnoreResult() {}

  /// Asserts that the result indicates success.
  ///
  /// If the result is failure, the program terminates.
  /// \param szMsg Optional short message for assertion failure.
  /// \param szDetails Optional detailed message, used in conjunction with \a szMsg.
  /// If \a szDetails is provided, \a szMsg should contain a formatting placeholder (`{}`).
  void AssertSuccess(const char* szMsg = nullptr, const char* szDetails = nullptr) const;

  /// @}

private:
  xiiResultEnum m_E;
};

/// Explicit conversion to xiiResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
XII_ALWAYS_INLINE xiiResult xiiToResult(xiiResult result)
{
  return result;
}

/// Helper macro to call functions that return xiiStatus or xiiResult in a function that returns xiiStatus (or xiiResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define XII_SUCCEED_OR_RETURN(code) \
  do                                \
  {                                 \
    auto s = (code);                \
    if (xiiToResult(s).Failed())    \
      return s;                     \
  } while (false)

/// Like XII_SUCCEED_OR_RETURN, but with error logging.
#define XII_SUCCEED_OR_RETURN_LOG(code)                                        \
  do                                                                           \
  {                                                                            \
    auto s = (code);                                                           \
    if (xiiToResult(s).Failed())                                               \
    {                                                                          \
      xiiLog::Error("Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), s); \
      return s;                                                                \
    }                                                                          \
  } while (false)

/// Like XII_SUCCEED_OR_RETURN, but with custom error logging.
#define XII_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                              \
  do                                                                             \
  {                                                                              \
    auto s = (code);                                                             \
    if (xiiToResult(s).Failed())                                                 \
    {                                                                            \
      xiiLog::Error("Call '{0}' failed with: {1}", XII_PP_STRINGIFY(code), log); \
      return s;                                                                  \
    }                                                                            \
  } while (false)

//////////////////////////////////////////////////////////////////////////

class xiiRTTI;

/// Dummy type to pass to templates and macros that expect a base type for a class that has no base.
class xiiNoBase
{
public:
  static const xiiRTTI* GetStaticRTTI() { return nullptr; }
};

/// Dummy type to pass to templates and macros that expect a base type for an enum class.
class xiiEnumBase
{
};

/// Dummy type to pass to templates and macros that expect a base type for an bitflags class.
class xiiBitflagsBase
{
};

/// Helper struct to get a storage type from a size in byte.
template <size_t SizeInByte>
struct xiiSizeToType;
/// \cond
template <>
struct xiiSizeToType<1>
{
  using Type = xiiUInt8;
};
template <>
struct xiiSizeToType<2>
{
  using Type = xiiUInt16;
};
template <>
struct xiiSizeToType<3>
{
  using Type = xiiUInt32;
};
template <>
struct xiiSizeToType<4>
{
  using Type = xiiUInt32;
};
template <>
struct xiiSizeToType<5>
{
  using Type = xiiUInt64;
};
template <>
struct xiiSizeToType<6>
{
  using Type = xiiUInt64;
};
template <>
struct xiiSizeToType<7>
{
  using Type = xiiUInt64;
};
template <>
struct xiiSizeToType<8>
{
  using Type = xiiUInt64;
};
/// \endcond
