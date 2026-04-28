# Copyright (c) Theophilus Eriata. All Rights Reserved.

# find the folder in which Embree is located

# early out, if this target has been created before
if(TARGET XIIEmbree::XIIEmbree)
  return()
endif()

find_path(XII_EMBREE_DIR include/embree3/rtcore.h
  PATHS
  ${XII_ROOT}/Source/ThirdParty/embree
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(EMBREE_LIB_PATH "${XII_EMBREE_DIR}/vc141win64")
else()
  set(EMBREE_LIB_PATH "${XII_EMBREE_DIR}/vc141win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XIIEmbree DEFAULT_MSG XII_EMBREE_DIR)

if(XIIEMBREE_FOUND)
  add_library(XIIEmbree::XIIEmbree SHARED IMPORTED)
  set_target_properties(XIIEmbree::XIIEmbree PROPERTIES IMPORTED_LOCATION "${EMBREE_LIB_PATH}/embree3.dll")
  set_target_properties(XIIEmbree::XIIEmbree PROPERTIES IMPORTED_IMPLIB "${EMBREE_LIB_PATH}/embree3.lib")
  set_target_properties(XIIEmbree::XIIEmbree PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XII_EMBREE_DIR}/include")
endif()

mark_as_advanced(FORCE XII_EMBREE_DIR)

unset(EMBREE_LIB_PATH)
