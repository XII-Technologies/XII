# Copyright (c) Theophilus Eriata. All Rights Reserved.

# #####################################
# ## Vulkan support
# #####################################
set(XII_BUILD_VULKAN OFF CACHE BOOL "Build the Vulkan Graphics Device.")

# #####################################
# ## xii_requires_vulkan()
# #####################################
macro(xii_requires_vulkan)
  xii_requires(XII_CMAKE_PLATFORM_SUPPORTS_VULKAN)
  xii_requires(XII_BUILD_VULKAN)
  find_package(XIIVulkan REQUIRED)
endmacro()

# #####################################
# ## xii_link_target_dxc(<target>)
# #####################################
function(xii_link_target_dxc TARGET_NAME)
  xii_requires_vulkan()

  find_package(XIIVulkan REQUIRED)

  if(XIIVULKAN_FOUND)
    target_link_libraries(${TARGET_NAME} PRIVATE XIIVulkan::DXC)

    get_target_property(_dll_location XIIVulkan::DXC IMPORTED_LOCATION)

    if(NOT _dll_location STREQUAL "")
      add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:XIIVulkan::DXC> $<TARGET_FILE_DIR:${TARGET_NAME}>)
    endif()

    unset(_dll_location)
  endif()
endfunction()
