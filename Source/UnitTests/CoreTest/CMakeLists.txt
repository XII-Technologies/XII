# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Core
  Texture
)

xii_project_build_filter_index(${PROJECT_NAME} BUILD_FILTER_IDX)

if(TARGET TextureConverter)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_TEXTURE_CONVERTER_PRESENT)
  
  add_dependencies(${PROJECT_NAME}
    TextureConverter
  )
endif()

xii_ci_add_test(${PROJECT_NAME})
