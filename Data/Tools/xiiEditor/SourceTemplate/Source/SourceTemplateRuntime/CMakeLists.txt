xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME} NO_XII_PREFIX NO_UNITY)
xii_make_windowapp(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

# include the XII natvis file for a better debugging experience
get_property(XII_ROOT_DIR GLOBAL PROPERTY XII_ROOT)
target_sources(${PROJECT_NAME} PUBLIC "${XII_ROOT_DIR}/Source/Engine/Foundation/xiiEngine.natvis")

target_compile_definitions(${PROJECT_NAME} PRIVATE "RUNTIME_PROJECT_FOLDER=${CMAKE_CURRENT_LIST_DIR}/../../")

add_dependencies(${PROJECT_NAME} SourceTemplatePlugin)
