xii_cmake_init()

xii_requires_one_of(XII_BUILD_D3D11 XII_BUILD_VULKAN)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GraphicsCore
)

if (XII_BUILD_D3D11)
  xii_link_target_d3d11(${PROJECT_NAME})
endif()

if (XII_BUILD_VULKAN)
  target_include_directories(${PROJECT_NAME}
    PRIVATE
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/DirectXShaderCompiler"
  )

  if(XII_CMAKE_COMPILER_CLANG)
    target_compile_options(${PROJECT_NAME} PRIVATE -fms-extensions)
  endif()

  xii_requires_vulkan()
  xii_link_target_dxc(${PROJECT_NAME})
  xii_sources_target_spirv_reflect(${PROJECT_NAME})
endif()
