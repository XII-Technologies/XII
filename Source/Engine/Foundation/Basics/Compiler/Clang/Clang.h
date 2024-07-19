
#pragma once

#ifdef __clang__

#  undef XII_COMPILER_CLANG
#  define XII_COMPILER_CLANG XII_ON

#  define XII_ALWAYS_INLINE __attribute__((always_inline)) inline
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
#    define XII_FORCE_INLINE inline
#  else
#    define XII_FORCE_INLINE __attribute__((always_inline)) inline
#  endif

#  define XII_ALIGNMENT_OF(type) XII_COMPILE_TIME_MAX(__alignof(type), XII_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define XII_DEBUG_BREAK    \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif __has_builtin(__debugbreak)
#    define XII_DEBUG_BREAK \
      {                     \
        __debugbreak();     \
      }
#  else
#    include <signal.h>
#    if defined(SIGTRAP)
#      define XII_DEBUG_BREAK \
        {                     \
          raise(SIGTRAP);     \
        }
#    else
#      define XII_DEBUG_BREAK \
        {                     \
          raise(SIGABRT);     \
        }
#    endif
#  endif

#  define XII_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define XII_SOURCE_LINE     __LINE__
#  define XII_SOURCE_FILE     __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef XII_COMPILE_FOR_DEBUG
#    define XII_COMPILE_FOR_DEBUG XII_ON
#  endif

#  define XII_WARNING_PUSH()            _Pragma("clang diagnostic push")
#  define XII_WARNING_POP()             _Pragma("clang diagnostic pop")
#  define XII_WARNING_DISABLE_CLANG(_x) _Pragma(XII_PP_STRINGIFY(clang diagnostic ignored _x))

#else

#  define XII_WARNING_DISABLE_CLANG(_x)

#endif
