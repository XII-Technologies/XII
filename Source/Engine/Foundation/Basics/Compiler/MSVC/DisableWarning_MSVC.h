
#ifdef XII_MSVC_WARNING_NUMBER

#  if XII_ENABLED(XII_COMPILER_MSVC)

#    pragma warning(push)
#    pragma warning(disable \
                    : XII_MSVC_WARNING_NUMBER)

#  endif

#  undef XII_MSVC_WARNING_NUMBER

#endif
