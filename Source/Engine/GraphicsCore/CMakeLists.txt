# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  GraphicsFoundation
  Texture

  PRIVATE
  meshoptimizer
)

if (XII_3RDPARTY_OZZ_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ozz
  )
endif()

# This is needed for shader structure includes
target_include_directories(${PROJECT_NAME} PUBLIC "${XII_ROOT}/Data/Base")
