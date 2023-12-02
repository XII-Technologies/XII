xii_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GraphicsCore
  ozz
  Utilities
)

if (XII_3RDPARTY_IMGUI_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PUBLIC Imgui)
endif()

if (XII_CMAKE_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    FileservePlugin
  )

endif()
