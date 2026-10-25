# Copyright (c) Theophilus Eriata. All Rights Reserved.

xii_cmake_init()

xii_requires(XII_3RDPARTY_IMGUI_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsCore
  Imgui
)

# Shaders use the normal Base data mount and XII shader compiler/cache, including packaged Base data.
set(DEAR_IMGUI_SHADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Base/Shaders/GUI")
set(DEAR_IMGUI_SHADERS
  "${DEAR_IMGUI_SHADER_ROOT}/DearImguiPrimitives.xiiShader"
  "${DEAR_IMGUI_SHADER_ROOT}/DearImguiConstants.h"
)
target_sources(${PROJECT_NAME} PRIVATE ${DEAR_IMGUI_SHADERS})
set_source_files_properties(${DEAR_IMGUI_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/DearImgui.xiiPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
