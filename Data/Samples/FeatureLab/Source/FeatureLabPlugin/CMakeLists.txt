xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_XII_PREFIX NO_UNITY)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
  Utilities
)

# include the XII natvis file for a better debugging experience
get_property(XII_ROOT_DIR GLOBAL PROPERTY XII_ROOT)
target_sources(${PROJECT_NAME} PUBLIC "${XII_ROOT_DIR}/Source/Engine/Foundation/xiiEngine.natvis")
