# Enable support for the Open Asset Import Library (assimp).
set(XII_3RDPARTY_ASSIMP_SUPPORT ON CACHE BOOL "Enable support for the Open Asset Import Library.")
mark_as_advanced(FORCE XII_3RDPARTY_ASSIMP_SUPPORT)

# Enable Draco mesh compression support inside the Open Asset Import Library.
set(XII_3RDPARTY_ASSIMP_DRACO_SUPPORT OFF CACHE BOOL "Enable Draco mesh compression support in the Open Asset Import Library.")
mark_as_advanced(FORCE XII_3RDPARTY_ASSIMP_DRACO_SUPPORT)

macro(xii_requires_assimp)
  xii_requires(XII_3RDPARTY_ASSIMP_SUPPORT)
  xii_requires_one_of(XII_CMAKE_PLATFORM_LINUX XII_CMAKE_PLATFORM_WINDOWS)
endmacro()
