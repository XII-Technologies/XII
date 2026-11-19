# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC Foundation
  PRIVATE Core
)

if(TARGET GraphicsCore AND TARGET GraphicsFoundation)
  target_link_libraries(${PROJECT_NAME} PRIVATE GraphicsCore GraphicsFoundation)
  target_compile_definitions(${PROJECT_NAME} PRIVATE XII_MCP_HAS_RENDERING)
endif()
