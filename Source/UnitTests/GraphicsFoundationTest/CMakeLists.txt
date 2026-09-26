# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

# The testing environment is backend agnostic. Keep it explicit because the project generator
# intentionally preserves the per-backend source placeholders for IDE organization.
target_sources(${PROJECT_NAME} PRIVATE TestingEnvironment/Implementation/TestingEnvironment.cpp)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GraphicsFoundation
  Core
)

xii_add_renderers(${PROJECT_NAME})

xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)
