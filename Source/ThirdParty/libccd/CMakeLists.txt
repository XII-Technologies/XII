xii_cmake_init()

xii_requires(XII_3RDPARTY_MUJOCO_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_WARNINGS_AS_ERRORS)

xii_set_clib_build_flags(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

target_compile_definitions(${PROJECT_NAME} PUBLIC CCD_STATIC_DEFINE)

if(XII_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    /wd4267  # size_t conversion
    /wd4996  # Deprecated function
  )
endif()
