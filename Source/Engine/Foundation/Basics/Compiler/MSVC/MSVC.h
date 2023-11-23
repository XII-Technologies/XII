#pragma once

#if defined(_MSC_VER) && !defined(__clang__)

#  undef XII_COMPILER_MSVC
#  define XII_COMPILER_MSVC XII_ON

#  if __clang__ || __castxml__
#    undef XII_COMPILER_MSVC_CLANG
#    define XII_COMPILER_MSVC_CLANG XII_ON
#  else
#    undef XII_COMPILER_MSVC_PURE
#    define XII_COMPILER_MSVC_PURE XII_ON
#  endif

#  ifdef _DEBUG
#    undef XII_COMPILE_FOR_DEBUG
#    define XII_COMPILE_FOR_DEBUG XII_ON
#  endif


// Functions marked as XII_ALWAYS_INLINE will be inlined even in Debug builds, which means you will step over them in a debugger
#  define XII_ALWAYS_INLINE __forceinline

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
#    define XII_FORCE_INLINE inline
#  else
#    define XII_FORCE_INLINE __forceinline
#  endif

// Workaround for MSVC compiler issue with alignment determination of dependent types
#  define XII_ALIGNMENT_OF(type) XII_COMPILE_TIME_MAX(XII_ALIGNMENT_MINIMUM, XII_COMPILE_TIME_MIN(sizeof(type), __alignof(type)))

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG) || (_MSC_VER >= 1929 /* Was broken in early VS2019 but works again in VS2022 and later 2019 versions*/)

#    define XII_DEBUG_BREAK \
      {                     \
        __debugbreak();     \
      }

#  else

#    define XII_DEBUG_BREAK                        \
      {                                            \
        /* Declared with DLL export in Assert.h */ \
        MSVC_OutOfLine_DebugBreak();               \
      }

#  endif

#  if XII_ENABLED(XII_COMPILER_MSVC_CLANG)
#    define XII_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  else
#    define XII_SOURCE_FUNCTION __FUNCTION__
#  endif

#  define XII_SOURCE_LINE __LINE__
#  define XII_SOURCE_FILE __FILE__

// XII_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro.
// Unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
// https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
#  if _MSC_VER >= 1400 && XII_DISABLED(XII_COMPILER_MSVC_CLANG)
#    define XII_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, N, ...) N
#    define XII_VA_NUM_ARGS_REVERSE_SEQUENCE                                                                                                                                                      32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define XII_LEFT_PARENTHESIS (
#    define XII_RIGHT_PARENTHESIS )
#    define XII_VA_NUM_ARGS(...) XII_VA_NUM_ARGS_HELPER XII_LEFT_PARENTHESIS __VA_ARGS__, XII_VA_NUM_ARGS_REVERSE_SEQUENCE XII_RIGHT_PARENTHESIS
#  endif

#  define XII_WARNING_PUSH()           __pragma(warning(push))
#  define XII_WARNING_POP()            __pragma(warning(pop))
#  define XII_WARNING_DISABLE_MSVC(_x) __pragma(warning(disable \
                                                       : _x))

#else

#  define XII_WARNING_DISABLE_MSVC(_x)

#endif
