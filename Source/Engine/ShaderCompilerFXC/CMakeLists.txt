xii_cmake_init()

xii_requires(XII_BUILD_D3D11)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_link_target_d3d11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GraphicsCore
)
