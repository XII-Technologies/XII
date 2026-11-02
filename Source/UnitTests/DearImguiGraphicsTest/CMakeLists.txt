# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires_renderer()

xii_requires(XII_3RDPARTY_IMGUI_SUPPORT)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ../GraphicsFoundationTest/TestingEnvironment/Implementation/TestingEnvironment.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC DearImguiPlugin TestFramework)

xii_add_renderers(${PROJECT_NAME})

xii_link_thirdparty_sdl(${PROJECT_NAME})

xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)
