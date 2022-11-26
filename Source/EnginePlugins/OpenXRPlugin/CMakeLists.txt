xii_cmake_init()


xii_requires_openxr()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

if (NOT XII_OPENXR_PREVIEW_DIR STREQUAL "")
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT)
endif()

if (XII_CMAKE_PLATFORM_WINDOWS_DESKTOP AND XII_CMAKE_ARCHITECTURE_64BIT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT)
endif()
    
xii_link_target_openxr(${PROJECT_NAME})
