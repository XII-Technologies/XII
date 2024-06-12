#pragma once

/// \file

/// Global settings for how to to compile XII.
/// Modify these settings as you needed in your project.


#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef XII_COMPILE_ENGINE_AS_DLL
#  define XII_COMPILE_ENGINE_AS_DLL XII_ON
#else
#  undef XII_COMPILE_ENGINE_AS_DLL
#  define XII_COMPILE_ENGINE_AS_DLL XII_OFF
#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Shipping)

// Development checks like assert.
#  undef XII_COMPILE_FOR_DEVELOPMENT
#  define XII_COMPILE_FOR_DEVELOPMENT XII_OFF

// Performance profiling features.
#  undef XII_USE_PROFILING
#  define XII_USE_PROFILING XII_OFF

// Tracking of memory allocations.
#  undef XII_ALLOC_TRACKING_DEFAULT
#  define XII_ALLOC_TRACKING_DEFAULT xiiAllocatorTrackingMode::DoNotTrack

#else

// Development checks like assert.
#  undef XII_COMPILE_FOR_DEVELOPMENT
#  define XII_COMPILE_FOR_DEVELOPMENT XII_ON

// Performance profiling features.
#  undef XII_USE_PROFILING
#  define XII_USE_PROFILING XII_ON

// Tracking of memory allocations.
#  undef XII_ALLOC_TRACKING_DEFAULT
#  define XII_ALLOC_TRACKING_DEFAULT xiiAllocatorTrackingMode::AllocationStatsAndStacktraces

#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Debug)
#  undef XII_MATH_CHECK_FOR_NAN
#  define XII_MATH_CHECK_FOR_NAN XII_ON
#  undef XII_USE_STRING_VALIDATION
#  define XII_USE_STRING_VALIDATION XII_ON
#endif

/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define XII_GAMEOBJECT_VELOCITY XII_ON

/// Whether to use double precision mode for large coordinates support.
#undef XII_DOUBLE_PRECISION
#define XII_DOUBLE_PRECISION XII_OFF
