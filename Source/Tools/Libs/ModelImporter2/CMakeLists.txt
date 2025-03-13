xii_cmake_init()

xii_requires_assimp()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  GraphicsCore

  PRIVATE
  mikktspace
  ozz
  meshoptimizer
)

xii_link_target_assimp(${PROJECT_NAME})
