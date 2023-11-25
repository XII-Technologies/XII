#pragma once

#ifndef XII_CONCAT

/// \brief Concatenates two strings, even when the strings are macros themselves
#define XII_CONCAT(x,y) XII_CONCAT_HELPER(x,y)
#define XII_CONCAT_HELPER(x,y) XII_CONCAT_HELPER2(x,y)
#define XII_CONCAT_HELPER2(x,y) x##y

#endif

#ifndef XII_STRINGIZE

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define XII_STRINGIZE(str) XII_STRINGIZE_HELPER(str)
#define XII_STRINGIZE_HELPER(x) #x

#endif

#ifndef XII_ON

/// \brief Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_ON =

/// \brief Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_OFF !

/// \brief Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_ENABLED(x) (1 XII_CONCAT(x,=) 1)

/// \brief Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_DISABLED(x) (1 XII_CONCAT(x,=) 2)

/// \brief Checks whether x AND y are both defined as XII_ON or XII_OFF. Usually used to check whether configurations overlap, to issue an error.
#define XII_IS_NOT_EXCLUSIVE(x, y) ((1 XII_CONCAT(x,=) 1) == (1 XII_CONCAT(y,=) 1))

#endif
