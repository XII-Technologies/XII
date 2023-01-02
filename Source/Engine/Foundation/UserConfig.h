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
#  undef XII_USE_ALLOCATION_TRACKING
#  define XII_USE_ALLOCATION_TRACKING XII_OFF

// Stack traces for memory allocations.
#  undef XII_USE_ALLOCATION_STACK_TRACING
#  define XII_USE_ALLOCATION_STACK_TRACING XII_OFF

#else

// Development checks like assert.
#  undef XII_COMPILE_FOR_DEVELOPMENT
#  define XII_COMPILE_FOR_DEVELOPMENT XII_ON

// Performance profiling features.
#  undef XII_USE_PROFILING
#  define XII_USE_PROFILING XII_ON

// Tracking of memory allocations.
#  undef XII_USE_ALLOCATION_TRACKING
#  define XII_USE_ALLOCATION_TRACKING XII_ON

// Stack traces for memory allocations.
#  undef XII_USE_ALLOCATION_STACK_TRACING
#  define XII_USE_ALLOCATION_STACK_TRACING XII_ON

#endif

/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define XII_GAMEOBJECT_VELOCITY XII_ON

/// Whether to use double precision mode for large coordinates support.
#undef XII_DOUBLE_PRECISION
#define XII_DOUBLE_PRECISION XII_ON
