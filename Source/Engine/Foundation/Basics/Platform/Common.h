/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
XII_WARNING_PUSH()
XII_WARNING_DISABLE_MSVC(4985)

// Include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

XII_WARNING_POP()

// Redefine NULL to nullptr
#undef NULL
#define NULL nullptr

// Include c++11 specific header
#include <type_traits>
#include <utility>

/// Disallow the copy constructor and the assignment operator for this type.
#define XII_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&)           = delete;    \
  void operator=(const type&) = delete

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
/// Macro helper to check alignment
#  define XII_CHECK_ALIGNMENT(ptr, alignment) XII_ASSERT_DEV(((size_t)ptr & ((alignment) - 1)) == 0, "Wrong alignment.")
#else
/// Macro helper to check alignment
#  define XII_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define XII_WINCHECK_1          1 // XII_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringified to nothing)
#define XII_WINCHECK_1_WINDOWS_ 1 // XII_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringified to "_WINDOWS_")
#define XII_WINCHECK_XII_INCLUDED_WINDOWS_H \
  0 // XII_INCLUDED_WINDOWS_H undefined (stringified to "XII_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringified to nothing)
#define XII_WINCHECK_XII_INCLUDED_WINDOWS_H_WINDOWS_ \
  1 // XII_INCLUDED_WINDOWS_H undefined (stringified to "XII_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringified to "_WINDOWS_")

/// Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define XII_CHECK_WINDOWS_INCLUDE(XII_WINH_INCLUDED, WINH_INCLUDED)                                 \
  static_assert(XII_PP_CONCAT(XII_WINCHECK_, XII_PP_CONCAT(XII_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
                "Windows.h has been included but not through XII. #include <Foundation/Basics/Platform/Windows/IncludeWindows.h> instead of Windows.h");

#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

/// The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of XII_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define XII_STATICLINK_FILE(LibraryName, UniqueName) XII_CHECK_WINDOWS_INCLUDE(XII_INCLUDED_WINDOWS_H, _WINDOWS_)

/// Used by the tool 'StaticLinkUtil' to generate the block after XII_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see XII_STATICLINK_FILE
#  define XII_STATICLINK_REFERENCE(UniqueName)

/// This must occur exactly once in each static library, such that all XII_STATICLINK_FILE macros can reference it.
#  define XII_STATICLINK_LIBRARY(LibraryName) void xiiReferenceFunction_##LibraryName(bool bReturn = true)

/// Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// XII_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define XII_STATICLINK_PLUGIN(PluginName)

/// A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example a xiiEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define XII_STATICLINK_FORCE

#else

struct xiiStaticLinkHelper
{
  using Func = void (*)(bool);
  xiiStaticLinkHelper(Func f) { f(true); }
};

/// Helper struct to register the existence of statically linked plugins.
/// The macro XII_STATICLINK_LIBRARY will register a the given library name prepended with `xii` to the xiiPlugin system.
/// Implemented in Plugin.cpp.
struct XII_FOUNDATION_DLL xiiPluginRegister
{
  xiiPluginRegister(const char* szName);
};

/// The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of XII_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define XII_STATICLINK_FILE(LibraryName, UniqueName)       \
    extern "C"                                               \
    {                                                        \
      void xiiReferenceFunction_##UniqueName(bool bReturn)   \
      {                                                      \
        (void)bReturn;                                       \
      }                                                      \
      void xiiReferenceFunction_##LibraryName(bool bReturn); \
    }                                                        \
    static xiiStaticLinkHelper StaticLinkHelper_##UniqueName(xiiReferenceFunction_##LibraryName);

/// Used by the tool 'StaticLinkUtil' to generate the block after XII_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see XII_STATICLINK_FILE
#  define XII_STATICLINK_REFERENCE(UniqueName)                   \
    void xiiReferenceFunction_##UniqueName(bool bReturn = true); \
    xiiReferenceFunction_##UniqueName()

/// This must occur exactly once in each static library, such that all XII_STATICLINK_FILE macros can reference it.
#  define XII_STATICLINK_LIBRARY(LibraryName)                                                             \
    xiiPluginRegister xiiPluginRegister_##LibraryName(XII_PP_STRINGIFY(XII_PP_CONCAT(xii, LibraryName))); \
    extern "C" void   xiiReferenceFunction_##LibraryName(bool bReturn = true)

/// Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// XII_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define XII_STATICLINK_PLUGIN(PluginName)                                                    \
    extern "C" void     XII_PP_CONCAT(xiiReferenceFunction_, PluginName)(bool bReturn = true); \
    xiiStaticLinkHelper XII_PP_CONCAT(xiiStaticLinkHelper_, PluginName)(XII_PP_CONCAT(xiiReferenceFunction_, PluginName));

/// A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example a xiiEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define XII_STATICLINK_FORCE

#endif

namespace xiiInternal
{
  template <typename T>
  constexpr bool AlwaysFalse = false;

  template <typename T>
  struct ArraySizeHelper
  {
    static_assert(AlwaysFalse<T>, "Cannot take compile time array size of given type.");
  };

  template <typename T, size_t N>
  struct ArraySizeHelper<T[N]>
  {
    static constexpr size_t value = N;
  };

} // namespace xiiInternal

/// Macro to determine the size of a static array
#define XII_ARRAY_SIZE(a) (xiiInternal::ArraySizeHelper<decltype(a)>::value)

/// Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void XII_IGNORE_UNUSED(const T&)
{
}
