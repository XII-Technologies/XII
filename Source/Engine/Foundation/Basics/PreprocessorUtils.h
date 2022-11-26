
#pragma once

/// \file

/// \brief Concatenates two strings, even when the strings are macros themselves
#define XII_CONCAT(x, y)         XII_CONCAT_HELPER(x, y)
#define XII_CONCAT_HELPER(x, y)  XII_CONCAT_HELPER2(x, y)
#define XII_CONCAT_HELPER2(x, y) x##y

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define XII_STRINGIZE(str)      XII_STRINGIZE_HELPER(str)
#define XII_STRINGIZE_HELPER(x) #x

/// \brief Concatenates two strings, even when the strings are macros themselves
#define XII_PP_CONCAT(x, y) XII_CONCAT_HELPER(x, y)

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define XII_PP_STRINGIFY(str) XII_STRINGIZE_HELPER(str)

/// \brief Max value of two compile-time constant expression.
#define XII_COMPILE_TIME_MAX(a, b) ((a) > (b) ? (a) : (b))

/// \brief Min value of two compile-time constant expression.
#define XII_COMPILE_TIME_MIN(a, b) ((a) < (b) ? (a) : (b))


/// \brief Creates a bit mask with only the n-th Bit set. Useful when creating enum values for flags.
#define XII_BIT(n) (1ull << (n))
