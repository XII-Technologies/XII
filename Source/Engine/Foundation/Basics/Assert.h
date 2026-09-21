/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifndef XII_INCLUDING_BASICS_H
#  error "Assert.h must not be included directly, but instead include Foundation/Basics.h."
#endif

/// \file

/// ***** Assert Usage Guidelines *****
///
/// For your typical code, use XII_ASSERT_DEV to check that vital preconditions are met.
/// Be aware that XII_ASSERT_DEV is removed in non-development builds (ie. when XII_COMPILE_FOR_DEVELOPMENT is disabled),
/// INCLUDING your code in the assert condition.
/// If the code that you are checking must be executed, even in non-development builds, use XII_VERIFY instead.
/// XII_ASSERT_DEV and XII_VERIFY will trigger a breakpoint in debug builds, but will not interrupt the application
/// in release builds.
///
/// For conditions that are rarely violated or checking is very costly, use XII_ASSERT_DEBUG. This assert is only active
/// in debug builds. This allows to have extra checking while debugging a program, but not waste performance when a
/// development or release build is used.
///
/// If you need to check something that is so vital that the application can only fail (i.e. crash), if that condition
/// is not met, even in release builds, then use XII_ASSERT_RELEASE. This should not be used in frequently executed code,
/// as it is not stripped from non-development builds by default.
///
/// If you need to squeeze the last bit of performance out of your code, XII_ASSERT_RELEASE can be disabled, by defining
/// XII_DISABLE_RELEASE_ASSERTS.
/// Please be aware that XII_ASSERT_RELEASE works like the other asserts, i.e. once it is deactivated, the code in the condition
/// is not executed anymore.
///

class xiiFormatString;

/// Assert handler callback. Should return true to trigger a break point or false if the assert should be ignored
using xiiAssertHandler = bool (*)(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

XII_FOUNDATION_DLL bool xiiDefaultAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

/// Gets the current assert handler. The default assert handler shows a dialog on windows or prints to the console on other platforms.
XII_FOUNDATION_DLL xiiAssertHandler xiiGetAssertHandler();

/// Sets the assert handler. It is the responsibility of the user to chain assert handlers if needed.
XII_FOUNDATION_DLL void xiiSetAssertHandler(xiiAssertHandler handler);

/// Called by the assert macros whenever a check failed. Returns true if the user wants to trigger a break point
XII_FOUNDATION_DLL bool xiiFailedCheck(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const class xiiFormatString& msg);
XII_FOUNDATION_DLL bool xiiFailedCheck(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg);

/// Dummy version of xiiFmt that only takes a single argument
inline const char* xiiFmt(const char* szFormat)
{
  return szFormat;
}

#if XII_ENABLED(XII_COMPILER_MSVC)
// Hides the call to __debugbreak from MSVCs optimizer to work around a bug in VS 2019
// that can lead to code (memcpy) after an assert to be omitted
XII_FOUNDATION_DLL void MSVC_OutOfLine_DebugBreak(...);
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
[[noreturn]] void ClangTidyDoNotReturn();
#  define XII_REPORT_FAILURE(szErrorMsg, ...) ClangTidyDoNotReturn()
#else
/// Macro to report a failure when that code is reached. This will ALWAYS be executed, even in release builds, therefore might crash the
/// application (or trigger a debug break).
#  define XII_REPORT_FAILURE(szErrorMsg, ...)                                                                           \
    do                                                                                                                  \
    {                                                                                                                   \
      if (xiiFailedCheck(XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, "", xiiFmt(szErrorMsg, ##__VA_ARGS__))) \
        XII_DEBUG_BREAK;                                                                                                \
    } while (false)
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
#  define XII_ASSERT_ALWAYS(bCondition, szErrorMsg, ...) \
    do                                                   \
    {                                                    \
      if (!!(bCondition) == false)                       \
        ClangTidyDoNotReturn();                          \
    } while (false)

#  define XII_ANALYSIS_ASSUME(bCondition) XII_ASSERT_ALWAYS(bCondition, "")
#else
/// Macro to raise an error, if a condition is not met. Allows to write a message using xiiFormatString style. This assert will be triggered, even in
/// non-development builds and cannot be deactivated.
#  define XII_ASSERT_ALWAYS(bCondition, szErrorMsg, ...)                                                                           \
    do                                                                                                                             \
    {                                                                                                                              \
      XII_MSVC_ANALYSIS_WARNING_PUSH                                                                                               \
      XII_MSVC_ANALYSIS_WARNING_DISABLE(6326) /* disable static analysis for the comparison */                                     \
      if (!!(bCondition) == false)                                                                                                 \
      {                                                                                                                            \
        if (xiiFailedCheck(XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, #bCondition, xiiFmt(szErrorMsg, ##__VA_ARGS__))) \
          XII_DEBUG_BREAK;                                                                                                         \
      }                                                                                                                            \
      XII_MSVC_ANALYSIS_WARNING_POP                                                                                                \
    } while (false)

/// Macro to inform the static analysis that the given condition can be assumed to be true. Useful to give additional information to
/// static analysis if it can't figure it out by itself. Will do nothing outside of static analysis runs.
#  define XII_ANALYSIS_ASSUME(bCondition)
#endif

/// This type of assert can be used to mark code as 'not (yet) implemented' and makes it easier to find it later on by just searching for these
/// asserts.
#define XII_ASSERT_NOT_IMPLEMENTED XII_REPORT_FAILURE("Not implemented")

// Occurrences of XII_ASSERT_DEBUG are compiled out in non-debug builds
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define XII_ASSERT_DEBUG XII_ASSERT_ALWAYS
#else
/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define XII_ASSERT_DEBUG(bCondition, szErrorMsg, ...)
#endif


// Occurrences of XII_ASSERT_DEV are compiled out in non-development builds
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define XII_ASSERT_DEV XII_ASSERT_ALWAYS

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define XII_VERIFY XII_ASSERT_ALWAYS

#else

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define XII_ASSERT_DEV(bCondition, szErrorMsg, ...)

/// Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using xiiFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define XII_VERIFY(bCondition, szErrorMsg, ...)                            \
    if (!!(bCondition) == false)                                             \
    { /* The condition is evaluated, even though nothing is done with it. */ \
    }

#endif

#if XII_DISABLE_RELEASE_ASSERTS

/// An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining XII_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define XII_ASSERT_RELEASE(bCondition, szErrorMsg, ...)

#else

/// An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining XII_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define XII_ASSERT_RELEASE XII_ASSERT_ALWAYS

#endif

/// Macro to make unhandled cases in a switch block an error.
#define XII_DEFAULT_CASE_NOT_IMPLEMENTED \
  default:                               \
    XII_ASSERT_NOT_IMPLEMENTED;          \
    break
