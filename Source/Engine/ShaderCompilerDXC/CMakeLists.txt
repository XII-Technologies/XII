xii_cmake_init()


xii_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

if (XII_CMAKE_PLATFORM_WINDOWS)
    xii_link_target_dx11(${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)

xii_link_target_dxc(${PROJECT_NAME})
xii_link_target_vulkan(${PROJECT_NAME})
xii_sources_target_spirv_reflect(${PROJECT_NAME})

if (XII_CMAKE_PLATFORM_WINDOWS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

if(XII_CMAKE_COMPILER_CLANG OR XII_CMAKE_COMPILER_GCC)
    set_source_files_properties("${XII_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.c" PROPERTIES COMPILE_FLAGS "-Wno-switch")
endif()

if(XII_CMAKE_COMPILER_CLANG)
   target_compile_options(${PROJECT_NAME} PRIVATE -fms-extensions)
endif()
