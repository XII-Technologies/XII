### Assimp
set (XII_3RDPARTY_ASSIMP_SUPPORT ON CACHE BOOL "Whether to add support for the asset importer.")
mark_as_advanced(FORCE XII_3RDPARTY_ASSIMP_SUPPORT)

set (XII_3RDPARTY_ASSIMP_DRACO_SUPPORT OFF CACHE BOOL "Whether to include Draco mesh compression support in AssImp.")
mark_as_advanced(FORCE XII_3RDPARTY_ASSIMP_DRACO_SUPPORT)

macro(xii_requires_assimp)
	
	xii_requires(XII_3RDPARTY_ASSIMP_SUPPORT)
	xii_requires_one_of(XII_CMAKE_PLATFORM_LINUX XII_CMAKE_PLATFORM_WINDOWS_DESKTOP)

endmacro()
