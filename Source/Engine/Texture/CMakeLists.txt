# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsFoundation

  PRIVATE
  DirectX-Headers
  DirectXTex
  DirectXMath
  stb
)

if(XII_CMAKE_PLATFORM_LINUX)
  target_link_libraries(${PROJECT_NAME} PRIVATE sal)

  if(XII_3RDPARTY_BC7ENC_RDO_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE bc7enc_rdo)
  endif()
endif()

if(XII_3RDPARTY_TINYEXR_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PRIVATE tinyexr)
endif()
