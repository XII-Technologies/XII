xii_cmake_init()

xii_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

#TODO: Add actual packaging code. This is done in PRE_BUILD so that it happens before the
#apk gen steps that happen in POST_BUILD and which are already done via xii_create_target.

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GraphicsFoundation
  Core
)

xii_add_renderers(${PROJECT_NAME} EXCLUDE_SHADER_COMPILER)

xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)
