# Copyright (c) Theophilus Eriata. All Rights Reserved.
xii_cmake_init()
xii_requires_desktop()
xii_requires_qt()
xii_requires(XII_3RDPARTY_ADS_SUPPORT)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
xii_create_target(APPLICATION ${PROJECT_NAME})
xii_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Tools)
target_sources(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/Source/Tools/Inspector/Implementation/RenderingCapture.cpp
  ${CMAKE_SOURCE_DIR}/Source/Tools/Inspector/Implementation/RenderGraphWidget.cpp
  ${CMAKE_SOURCE_DIR}/Source/Tools/Inspector/Implementation/ProfilingWidget.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE TestFramework GuiFoundation GraphicsCore ads)
add_dependencies(${PROJECT_NAME} InspectorPlugin)
if(XII_BUILD_VULKAN OR XII_BUILD_D3D12)
  target_sources(${PROJECT_NAME} PRIVATE ../GraphicsFoundationTest/TestingEnvironment/Implementation/TestingEnvironment.cpp)
  xii_add_renderers(${PROJECT_NAME})
  target_compile_definitions(${PROJECT_NAME} PRIVATE XII_INSPECTOR_GPU_TESTS)
endif()
xii_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)
