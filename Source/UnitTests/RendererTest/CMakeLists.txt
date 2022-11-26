xii_cmake_init()

xii_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  RendererCore
)

xii_add_renderers(${PROJECT_NAME})

xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

