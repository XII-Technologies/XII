#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
#define XII_MSVC_WARNING_NUMBER 4985
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

// Include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

// Redefine NULL to nullptr
#undef NULL
#define NULL nullptr

// Include c++11 specific header
#include <type_traits>
#include <utility>

#ifndef __has_cpp_attribute
#  define __has_cpp_attribute(name) 0
#endif

// [[nodiscard]] helper
#if __has_cpp_attribute(nodiscard)
#  define XII_NODISCARD [[nodiscard]]
#else
#  define XII_NODISCARD
#endif

#ifndef __INTELLISENSE__

// Macros to do compile-time checks, such as to ensure sizes of types
// XII_CHECK_AT_COMPILETIME(exp) : only checks exp
// XII_CHECK_AT_COMPILETIME_MSG(exp, msg) : checks exp and displays msg
#  define XII_CHECK_AT_COMPILETIME(exp) static_assert(exp, XII_STRINGIZE(exp) " is false.");

#  define XII_CHECK_AT_COMPILETIME_MSG(exp, msg) static_assert(exp, XII_STRINGIZE(exp) " is false. Message: " msg);

#else

// IntelliSense often isn't smart enough to evaluate these conditions correctly

#  define XII_CHECK_AT_COMPILETIME(exp)

#  define XII_CHECK_AT_COMPILETIME_MSG(exp, msg)

#endif

/// \brief Disallow the copy constructor and the assignment operator for this type.
#define XII_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;              \
  void operator=(const type&) = delete

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
/// \brief Macro helper to check alignment
#  define XII_CHECK_ALIGNMENT(ptr, alignment) XII_ASSERT_DEV(((size_t)ptr & ((alignment)-1)) == 0, "Wrong alignment.")
#else
/// \brief Macro helper to check alignment
#  define XII_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define XII_CHECK_ALIGNMENT_16(ptr)  XII_CHECK_ALIGNMENT(ptr, 16)
#define XII_CHECK_ALIGNMENT_32(ptr)  XII_CHECK_ALIGNMENT(ptr, 32)
#define XII_CHECK_ALIGNMENT_64(ptr)  XII_CHECK_ALIGNMENT(ptr, 64)
#define XII_CHECK_ALIGNMENT_128(ptr) XII_CHECK_ALIGNMENT(ptr, 128)

#define XII_WINCHECK_1          1 // XII_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define XII_WINCHECK_1_WINDOWS_ 1 // XII_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define XII_WINCHECK_XII_INCLUDED_WINDOWS_H \
  0 // XII_INCLUDED_WINDOWS_H undefined (stringyfied to "XII_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define XII_WINCHECK_XII_INCLUDED_WINDOWS_H_WINDOWS_ \
  1 // XII_INCLUDED_WINDOWS_H undefined (stringyfied to "XII_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// \brief Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define XII_CHECK_WINDOWS_INCLUDE(XII_WINH_INCLUDED, WINH_INCLUDED)                                          \
  XII_CHECK_AT_COMPILETIME_MSG(XII_CONCAT(XII_WINCHECK_, XII_CONCAT(XII_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
                               "Windows.h has been included but not through XII. #include <Foundation/Basics/Platform/Win/IncludeWindows.h> instead of Windows.h");


/// \brief Define some macros to work with the MSVC analysis warning
/// Note that the StaticAnalysis.h in Basics/Compiler/MSVC will define the MSVC specific versions.
#define XII_MSVC_ANALYSIS_WARNING_PUSH
#define XII_MSVC_ANALYSIS_WARNING_POP
#define XII_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber)
#define XII_MSVC_ANALYSIS_ASSUME(expression)

#if defined(_MSC_VER)
#  include <Foundation/Basics/Compiler/MSVC/StaticAnalysis.h>
#endif

#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of XII_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define XII_STATICLINK_FILE(LibraryName, UniqueName) XII_CHECK_WINDOWS_INCLUDE(XII_INCLUDED_WINDOWS_H, _WINDOWS_)


/// \brief Used by the tool 'StaticLinkUtil' to generate the block after XII_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see XII_STATICLINK_FILE
#  define XII_STATICLINK_REFERENCE(UniqueName)

/// \brief This must occur exactly once in each static library, such that all XII_STATICLINK_FILE macros can reference it.
#  define XII_STATICLINK_LIBRARY(LibraryName) void xiiReferenceFunction_##LibraryName(bool bReturn = true)

#else

struct xiiStaticLinkHelper
{
  using Func = void (*)(bool);
  xiiStaticLinkHelper(Func f) { f(true); }
};

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of XII_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define XII_STATICLINK_FILE(LibraryName, UniqueName)                           \
    void xiiReferenceFunction_##UniqueName(bool bReturn)                         \
    {}                                                                           \
    void                       xiiReferenceFunction_##LibraryName(bool bReturn); \
    static xiiStaticLinkHelper StaticLinkHelper_##UniqueName(xiiReferenceFunction_##LibraryName);

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after XII_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see XII_STATICLINK_FILE
#  define XII_STATICLINK_REFERENCE(UniqueName)                   \
    void xiiReferenceFunction_##UniqueName(bool bReturn = true); \
    xiiReferenceFunction_##UniqueName()

/// \brief This must occur exactly once in each static library, such that all XII_STATICLINK_FILE macros can reference it.
#  define XII_STATICLINK_LIBRARY(LibraryName) void xiiReferenceFunction_##LibraryName(bool bReturn = true)

#endif

namespace xiiInternal
{
  template <typename T, size_t N>
  char (*ArraySizeHelper(T (&)[N]))[N];
}

/// \brief Macro to determine the size of a static array
#define XII_ARRAY_SIZE(a) (sizeof(*xiiInternal::ArraySizeHelper(a)) + 0)

/// \brief Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void XII_IGNORE_UNUSED(const T&)
{
}


// Math Debug checks
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)

#  undef XII_MATH_CHECK_FOR_NAN
#  define XII_MATH_CHECK_FOR_NAN XII_ON

#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define XII_DECL_EXPORT        __declspec(dllexport)
#  define XII_DECL_IMPORT        __declspec(dllimport)
#  define XII_DECL_EXPORT_FRIEND __declspec(dllexport)
#  define XII_DECL_IMPORT_FRIEND __declspec(dllimport)
#else
#  define XII_DECL_EXPORT [[gnu::visibility("default")]]
#  define XII_DECL_IMPORT [[gnu::visibility("default")]]
#  define XII_DECL_EXPORT_FRIEND
#  define XII_DECL_IMPORT_FRIEND
#endif
