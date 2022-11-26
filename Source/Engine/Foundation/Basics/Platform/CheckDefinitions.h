#pragma once

#ifndef NULL
#  error "NULL is not defined."
#endif

#ifndef XII_FORCE_INLINE
#  error "XII_FORCE_INLINE is not defined."
#endif

#ifndef XII_ALWAYS_INLINE
#  error "XII_ALWAYS_INLINE is not defined."
#endif

#ifndef XII_ALIGNMENT_OF
#  error "XII_ALIGNMENT_OF is not defined."
#endif

#if XII_IS_NOT_EXCLUSIVE(XII_PLATFORM_32BIT, XII_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#ifndef XII_DEBUG_BREAK
#  error "XII_DEBUG_BREAK is not defined."
#endif

#ifndef XII_SOURCE_FUNCTION
#  error "XII_SOURCE_FUNCTION is not defined."
#endif

#ifndef XII_SOURCE_FILE
#  error "XII_SOURCE_FILE is not defined."
#endif

#ifndef XII_SOURCE_LINE
#  error "XII_SOURCE_LINE is not defined."
#endif

#if XII_IS_NOT_EXCLUSIVE(XII_PLATFORM_LITTLE_ENDIAN, XII_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef XII_MATH_CHECK_FOR_NAN
#  error "XII_MATH_CHECK_FOR_NAN is not defined."
#endif

#if XII_IS_NOT_EXCLUSIVE(XII_PLATFORM_ARCH_X86, XII_PLATFORM_ARCH_ARM)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(XII_SIMD_IMPLEMENTATION) || (XII_SIMD_IMPLEMENTATION == 0)
#  error "XII_SIMD_IMPLEMENTATION is not correctly defined."
#endif
