# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_d3d12()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

xii_link_target_d3d12(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  GraphicsFoundation
  D3D12MemoryAllocator
)

if("${XII_CMAKE_WINDOWS_SDK_VERSION}" VERSION_GREATER_EQUAL "10.0.19041.0")
  target_compile_definitions(${PROJECT_NAME} PRIVATE D3D12_H_HAS_MESH_SHADER=1)
endif()

if (XII_3RDPARTY_SDL_SUPPORT)
  if(XII_COMPILE_ENGINE_AS_DLL)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-shared)
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-static)
  endif()
endif()
