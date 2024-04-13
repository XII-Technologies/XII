xii_cmake_init()

xii_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

xii_create_target(LIBRARY ${PROJECT_NAME})

xii_enable_strict_warnings(${PROJECT_NAME})

xii_link_target_vulkan(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  GraphicsFoundation
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "${CMAKE_SOURCE_DIR}/Source/ThirdParty/VulkanMemoryAllocator"
)
