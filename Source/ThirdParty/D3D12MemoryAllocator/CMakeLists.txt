xii_cmake_init()

xii_requires_d3d12()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(STATIC_LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  DirectX-Headers
)

target_compile_definitions(${PROJECT_NAME} PRIVATE D3D12MA_USING_DIRECTX_HEADERS)
