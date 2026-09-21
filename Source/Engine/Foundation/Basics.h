/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#define XII_INCLUDING_BASICS_H

// Basic pre-processor definitions.
#include <Foundation/Basics/PreprocessorUtils.h>

// Set all feature definitions to XII_OFF.
#include <Foundation/Basics/AllDefinesOff.h>

// General OS and Hardware detection.
#include <Foundation/Basics/Platform/DetectArchitecture.h>
#include <Foundation/Basics/Platform/DetectPlatform.h>

// Here all the different features that each platform supports are declared.
#include <Foundation/Basics/Platform/PlatformFeatures.h>

// Build override definitions.
#include <Foundation/UserConfig.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#    define XII_FOUNDATION_DLL        XII_DECL_EXPORT
#    define XII_FOUNDATION_DLL_FRIEND XII_DECL_EXPORT_FRIEND
#  else
#    define XII_FOUNDATION_DLL        XII_DECL_IMPORT
#    define XII_FOUNDATION_DLL_FRIEND XII_DECL_IMPORT_FRIEND
#  endif
#else
#  define XII_FOUNDATION_DLL
#  define XII_FOUNDATION_DLL_FRIEND
#endif

#include <Foundation/FoundationInternal.h>

// Include headers for the supported compilers.
#include <Foundation/Basics/Compiler/Clang.h>
#include <Foundation/Basics/Compiler/GCC.h>
#include <Foundation/Basics/Compiler/MSVC.h>

// Include common definitions and macros (e.g. static_assert).
#include <Foundation/Basics/Platform/Common.h>

// Include magic preprocessor macros.
#include <Foundation/Basics/Platform/BlackMagic.h>

// Now declare all fundamental types.
#include <Foundation/Types/Types.h>

// Assert macros should always be available.
#include <Foundation/Basics/Assert.h>

// Type-trait utilities.
#include <Foundation/Types/TypeTraits.h>

// Memory allocators are needed.
#include <Foundation/Memory/Allocator.h>

// String formatting is needed by the asserts.
#include <Foundation/Strings/FormatString.h>

class XII_FOUNDATION_DLL xiiFoundation
{
public:
  static xiiAllocator* s_pDefaultAllocator;
  static xiiAllocator* s_pAlignedAllocator;

  /// The default allocator can be used for any kind of allocation if no alignment is required
  XII_ALWAYS_INLINE static xiiAllocator* GetDefaultAllocator()
  {
    if (s_bIsInitialized)
      return s_pDefaultAllocator;
    else // the default allocator is not yet set so we return the static allocator instead.
      return GetStaticAllocator();
  }

  /// The aligned allocator should be used for all allocations which need alignment
  XII_ALWAYS_INLINE static xiiAllocator* GetAlignedAllocator()
  {
    XII_ASSERT_RELEASE(s_pAlignedAllocator != nullptr, "xiiFoundation must have been initialized before this function can be called. This "
                                                       "error can occur when you have a global variable or a static member variable that "
                                                       "(indirectly) requires an allocator. Check out the documentation for 'xiiStatic' for "
                                                       "more information about this issue.");
    return s_pAlignedAllocator;
  }

  /// Returns the allocator that is used by global data and static members before the default allocator is created.
  static xiiAllocator* GetStaticAllocator();

private:
  friend class xiiStartup;
  friend struct xiiStaticAllocatorWrapper;

  static void Initialize();

  static bool s_bIsInitialized;
};

#undef XII_INCLUDING_BASICS_H
