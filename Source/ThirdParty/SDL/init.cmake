######################################
### SDL Support
######################################

set (XII_3RDPARTY_SDL_SUPPORT ON CACHE BOOL "Use SDL to manage windows and input")

if (XII_3RDPARTY_SDL_SUPPORT)
    message(STATUS "Using SDL Window and Input Handling by Default")
endif()
