######################################
### SDL Support
######################################

# On Linux SDL support is required
if(XII_CMAKE_PLATFORM_LINUX)
  # On Linux we want to use SDL by default
  set (XII_3RDPARTY_SDL_SUPPORT ON CACHE BOOL "Use SDL to manage windows and input")
  message(STATUS "Using SDL Window and Input Handling by Default")
else()
  # On all other platforms SDL support is optional.
  set (XII_3RDPARTY_SDL_SUPPORT OFF CACHE BOOL "Use SDL to manage windows and input")

  if (NOT XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    mark_as_advanced(FORCE XII_3RDPARTY_SDL_SUPPORT)
  endif()
endif()
