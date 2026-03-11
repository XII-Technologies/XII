///	Preprocessor utilities for shader sources.
///
///	- Concatenate / stringify helpers.
///	- Variadic argument counting and simple FOR_EACH mapper (up to 16 args).
///	- Convenience macros for language-target detection (HLSL).
///	- Include-as-string helpers and small utilities.
///
///	This header is designed to be safe to include in shader source files
///	that are processed by typical C-like preprocessors used by HLSL.

#pragma once

/// Concatenate two tokens (supports macro-expanded tokens).
#ifndef XII_PP_CONCAT
#  define XII_PP_CONCAT_IMPL(a, b) a##b
#  define XII_PP_CONCAT(a, b)      XII_PP_CONCAT_IMPL(a, b)
#endif

/// Stringify a token (supports macro-expanded tokens).
#ifndef XII_PP_STRINGIFY
#  define XII_PP_STRINGIFY_IMPL(x) #x
#  define XII_PP_STRINGIFY(x)      XII_PP_STRINGIFY_IMPL(x)
#endif

/// Empty helper.
#ifndef XII_PP_EMPTY
#  define XII_PP_EMPTY()
#endif

/// Expand helper - forces another macro expansion pass.
#ifndef XII_PP_EXPAND
#  define XII_PP_EXPAND(x) x
#endif

/// Variadic argument counting (works up to 16 args).
#ifndef XII_PP_ARG_COUNT
#  define XII_PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#  define XII_PP_RSEQ_N()                                                                             16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#  define XII_PP_ARG_COUNT_IMPL(...)                                                                  XII_PP_ARG_N(__VA_ARGS__)
#  define XII_PP_ARG_COUNT(...)                                                                       XII_PP_ARG_COUNT_IMPL(__VA_ARGS__, XII_PP_RSEQ_N())
#endif

/// FOR_EACH mapper (applies M to each arg) - supports up to 16 args.
#ifndef XII_PP_FOR_EACH
#  define XII_PP_FOR_EACH_1(M, x)       M(x)
#  define XII_PP_FOR_EACH_2(M, x, ...)  M(x) XII_PP_FOR_EACH_1(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_3(M, x, ...)  M(x) XII_PP_FOR_EACH_2(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_4(M, x, ...)  M(x) XII_PP_FOR_EACH_3(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_5(M, x, ...)  M(x) XII_PP_FOR_EACH_4(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_6(M, x, ...)  M(x) XII_PP_FOR_EACH_5(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_7(M, x, ...)  M(x) XII_PP_FOR_EACH_6(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_8(M, x, ...)  M(x) XII_PP_FOR_EACH_7(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_9(M, x, ...)  M(x) XII_PP_FOR_EACH_8(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_10(M, x, ...) M(x) XII_PP_FOR_EACH_9(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_11(M, x, ...) M(x) XII_PP_FOR_EACH_10(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_12(M, x, ...) M(x) XII_PP_FOR_EACH_11(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_13(M, x, ...) M(x) XII_PP_FOR_EACH_12(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_14(M, x, ...) M(x) XII_PP_FOR_EACH_13(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_15(M, x, ...) M(x) XII_PP_FOR_EACH_14(M, __VA_ARGS__)
#  define XII_PP_FOR_EACH_16(M, x, ...) M(x) XII_PP_FOR_EACH_15(M, __VA_ARGS__)

#  define XII_PP_FOR_EACH_CHOOSER2(count) XII_PP_CONCAT(XII_PP_FOR_EACH_, count)
#  define XII_PP_FOR_EACH_CHOOSER(count)  XII_PP_FOR_EACH_CHOOSER2(count)
#  define XII_PP_FOR_EACH(M, ...)         XII_PP_FOR_EACH_CHOOSER(XII_PP_ARG_COUNT(__VA_ARGS__))(M, __VA_ARGS__)
#endif

/// Convenience: include as string.
#ifndef XII_PP_INCLUDE_STR
/// Produce a quoted include string from a token (useful for #include XII_PP_INCLUDE_STR(path)).
#  define XII_PP_INCLUDE_STR(path) XII_PP_STRINGIFY(path)
#endif

/// On/Off style flags (safe checks).
///	 Usage:
///		 #define MY_FEATURE XII_ON
///		 #if XII_ENABLED(MY_FEATURE)
///			 // feature enabled
///		 #endif
#ifndef XII_ON
#  define XII_ON                     =
#  define XII_OFF                    !
#  define XII_ENABLED(x)             (1 XII_PP_CONCAT(x, =) 1)
#  define XII_DISABLED(x)            (1 XII_PP_CONCAT(x, =) 2)
#  define XII_IS_NOT_EXCLUSIVE(x, y) ((1 XII_PP_CONCAT(x, =) 1) == (1 XII_PP_CONCAT(y, =) 1))
#endif

#ifndef XII_SHADER_VERSION_MAJOR
#  define XII_SHADER_VERSION_MAJOR 0
#endif
#ifndef XII_SHADER_VERSION_MINOR
#  define XII_SHADER_VERSION_MINOR 0
#endif
#define XII_SHADER_VERSION_ENCODE(uiMajor, uiMinor) ((uiMajor) * 100 + (uiMinor))
#define XII_SHADER_VERSION                          XII_SHADER_VERSION_ENCODE(XII_SHADER_VERSION_MAJOR, XII_SHADER_VERSION_MINOR)


/// Misc utilities.
#ifndef XII_DEBUG
#  if defined(DEBUG) || defined(_DEBUG)
#    define XII_DEBUG 1
#  else
#    define XII_DEBUG 0
#  endif
#endif

/// Unused var hint (useful to silence warnings when same header used in C++ wrappers).
#ifndef XII_UNUSED
#  define XII_UNUSED(x) (void)(x)
#endif
