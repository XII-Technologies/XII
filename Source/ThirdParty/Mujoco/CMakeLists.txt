xii_cmake_init()

xii_requires(XII_3RDPARTY_MUJOCO_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_WARNINGS_AS_ERRORS)

xii_set_clib_build_flags(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  libccd
  lodepng
  libqhull_r
  MarchingCubeCpp
  tinyxml2
  miniz
)

if(XII_COMPILE_ENGINE_AS_DLL)
  target_compile_definitions(${PROJECT_NAME} PRIVATE MUJOCO_DLL_EXPORTS)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC MJ_STATIC)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include

  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}/src/xml
)

if(XII_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    /wd4018  # Signed/unsigned comparison
    /wd4146  # Unary minus on unsigned type
    /wd4244  # Numeric conversion
    /wd4267  # size_t conversion
    /wd4305  # Floating-point truncation
    /wd4101  # Unreferenced local variable
  )
endif()
