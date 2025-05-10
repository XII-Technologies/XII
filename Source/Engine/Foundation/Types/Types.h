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

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
enum xiiResultEnum
{
  XII_FAILURE,
  XII_SUCCESS
};

/// \brief Default enum for returning failure or success, instead of using a bool.
struct [[nodiscard]] XII_FOUNDATION_DLL xiiResult
{
public:
  xiiResult(xiiResultEnum res) :
    m_E(res)
  {
  }

  void operator=(xiiResultEnum rhs) { m_E = rhs; }
  bool operator==(xiiResultEnum cmp) const { return m_E == cmp; }

  [[nodiscard]] XII_ALWAYS_INLINE bool Succeeded() const { return m_E == XII_SUCCESS; }
  [[nodiscard]] XII_ALWAYS_INLINE bool Failed() const { return m_E == XII_FAILURE; }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  XII_ALWAYS_INLINE void IgnoreResult()
  {
    /* To be called when a return value is [[nodiscard]] but the result is not needed. */
  }

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message. If \a details is provided, \a msg should contain a formatting element ({}), e.g. "Error: {}".
  void AssertSuccess(const char* szMsg = nullptr, const char* szDetails = nullptr) const;

private:
  xiiResultEnum m_E;
};

/// \brief Explicit conversion to xiiResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
XII_ALWAYS_INLINE xiiResult xiiToResult(xiiResult result)
{
  return result;
}

/// \brief Helper macro to call functions that return xiiStatus or xiiResult in a function that returns xiiStatus (or xiiResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define XII_SUCCEED_OR_RETURN(code) \
  do                                \
  {                                 \
    auto s = (code);                \
    if (xiiToResult(s).Failed())    \
      return s;                     \
  } while (false)

/// \brief Like XII_SUCCEED_OR_RETURN, but with error logging.
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

/// \brief Like XII_SUCCEED_OR_RETURN, but with custom error logging.
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

/// \brief Dummy type to pass to templates and macros that expect a base type for a class that has no base.
class xiiNoBase
{
public:
  static const xiiRTTI* GetStaticRTTI() { return nullptr; }
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an enum class.
class xiiEnumBase
{
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an bitflags class.
class xiiBitflagsBase
{
};

/// \brief Helper struct to get a storage type from a size in byte.
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
