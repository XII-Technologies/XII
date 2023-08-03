
#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef XII_COMPILER_GCC
#  define XII_COMPILER_GCC XII_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define XII_ALWAYS_INLINE inline
#  define XII_FORCE_INLINE  inline

#  define XII_ALIGNMENT_OF(type) XII_COMPILE_TIME_MAX(__alignof(type), XII_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define XII_DEBUG_BREAK    \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif defined(__i386__) || defined(__x86_64__)
#    define XII_DEBUG_BREAK           \
      {                               \
        __asm__ __volatile__("int3"); \
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

#endif
