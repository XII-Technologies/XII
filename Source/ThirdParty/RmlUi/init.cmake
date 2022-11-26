### RmlUi
set (XII_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")
mark_as_advanced(FORCE XII_BUILD_RMLUI)

macro(xii_requires_rmlui)
  xii_requires_one_of(XII_CMAKE_PLATFORM_WINDOWS XII_CMAKE_PLATFORM_LINUX)
  xii_requires(XII_BUILD_RMLUI)
  if (XII_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()