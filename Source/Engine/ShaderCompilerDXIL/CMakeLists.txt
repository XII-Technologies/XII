# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_desktop()

xii_requires_d3d12()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GraphicsFoundation
  dxcompiler.lib
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/ThirdParty/DirectXShaderCompiler"
)

if(XII_CMAKE_COMPILER_CLANG)
  target_compile_options(${PROJECT_NAME} PRIVATE -fms-extensions)
endif()

xii_link_target_dxc(${PROJECT_NAME})
