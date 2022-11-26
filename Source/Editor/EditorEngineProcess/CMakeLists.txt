xii_cmake_init()

xii_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(APPLICATION ${PROJECT_NAME})

xii_make_winmain_executable(${PROJECT_NAME})

xii_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  EditorEngineProcessFramework
)

# For plugins to work in UWP builds, they must be direct link dependencies, otherwise they are not packaged with the app.
if (XII_CMAKE_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    InspectorPlugin
  )

  if (XII_BUILD_FMOD)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      FmodPlugin
    )

  endif()

  if (XII_BUILD_PHYSX)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      PhysXPlugin
      PhysXCooking
    )

  endif()
endif()
