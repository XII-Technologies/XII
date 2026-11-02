# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  Texture

  PRIVATE
  mikktspace
)

if (XII_3RDPARTY_LUA_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PUBLIC Lua)
endif()

xii_link_thirdparty_sdl(${PROJECT_NAME})
