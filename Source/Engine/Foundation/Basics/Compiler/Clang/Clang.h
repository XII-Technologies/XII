

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

#  define XII_DEBUG_BREAK \
    {                     \
      __builtin_trap();   \
    }

#  define XII_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define XII_SOURCE_LINE     __LINE__
#  define XII_SOURCE_FILE     __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_DEBUG
#    undef XII_COMPILE_FOR_DEBUG
#    define XII_COMPILE_FOR_DEBUG XII_ON
#  endif

#endif
