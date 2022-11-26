xii_cmake_init()

xii_requires_embree()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Utilities
)

xii_link_target_embree(${PROJECT_NAME})
