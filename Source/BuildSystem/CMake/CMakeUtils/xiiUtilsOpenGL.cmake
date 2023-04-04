# #####################################
# ## OpenGL/GLES support
# #####################################

set(XII_BUILD_OPENGL OFF CACHE BOOL "Build the OpenGL/GLES Graphics Device")

# #####################################
# ## xii_requires_opengl()
# #####################################

macro(xii_requires_opengl)
    xii_requires(XII_BUILD_OPENGL)
    xii_requires_one_of(XII_CMAKE_PLATFORM_WINDOWS XII_CMAKE_PLATFORM_LINUX XII_CMAKE_PLATFORM_ANDROID)
endmacro()
