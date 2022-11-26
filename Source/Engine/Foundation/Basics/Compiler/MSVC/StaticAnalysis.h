
#pragma once

#undef XII_MSVC_ANALYSIS_WARNING_PUSH
#undef XII_MSVC_ANALYSIS_WARNING_POP
#undef XII_MSVC_ANALYSIS_WARNING_DISABLE
#undef XII_MSVC_ANALYSIS_ASSUME

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#define XII_MSVC_ANALYSIS_WARNING_PUSH                   __pragma(warning(push))
#define XII_MSVC_ANALYSIS_WARNING_POP                    __pragma(warning(pop))
#define XII_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable \
                                                                          : warningNumber))
#define XII_MSVC_ANALYSIS_ASSUME(expression) __assume(expression)
