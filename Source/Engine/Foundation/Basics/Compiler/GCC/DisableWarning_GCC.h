
#ifdef XII_GCC_WARNING_NAME

#  if XII_ENABLED(XII_COMPILER_GCC)

#    pragma GCC diagnostic push
_Pragma(XII_STRINGIZE(GCC diagnostic ignored XII_GCC_WARNING_NAME))

#  endif

#  undef XII_GCC_WARNING_NAME

#endif
