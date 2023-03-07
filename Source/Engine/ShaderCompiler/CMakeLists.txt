xii_cmake_init()

xii_requires_one_of(XII_BUILD_D3D11 XII_BUILD_D3D12 XII_BUILD_VULKAN)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)

if (XII_BUILD_D3D11 OR XII_BUILD_D3D12)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    dxguid.lib
    d3dcompiler.lib
  )
endif()

if (XII_BUILD_VULKAN)
  # Call Find Vulkan first as that initializes resources that are needed
  xii_requires_vulkan()
  xii_link_target_dxc(${PROJECT_NAME})
  xii_sources_target_spirv_reflect(${PROJECT_NAME})
endif()
