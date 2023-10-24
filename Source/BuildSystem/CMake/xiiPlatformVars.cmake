# Cmake variables which are platform dependent

# #####################################
# ## General settings
# #####################################
if(XII_CMAKE_PLATFORM_WINDOWS OR XII_CMAKE_PLATFORM_LINUX)
  set(XII_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
  mark_as_advanced(FORCE XII_COMPILE_ENGINE_AS_DLL)
else()
  unset(XII_COMPILE_ENGINE_AS_DLL CACHE)
endif()

# #####################################
# ## Experimental Editor support on Linux
# #####################################
if(XII_CMAKE_PLATFORM_LINUX)
  set (XII_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")
endif()
