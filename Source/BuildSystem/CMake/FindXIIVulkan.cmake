# Find the folder into which the Vulkan SDK has been installed

# Early out, if this target has been created before
if((TARGET XIIVulkan::Loader) AND(TARGET XIIVulkan::DXC))
  return()
endif()

set(XII_VULKAN_DIR $ENV{VULKAN_SDK} CACHE PATH "Directory of the Vulkan SDK")

xii_pull_compiler_and_architecture_vars()
xii_pull_config_vars()

get_property(XII_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY XII_SUBMODULE_PREFIX_PATH)

if (COMMAND xii_platformhook_find_vulkan)
  xii_platformhook_find_vulkan()
else()
  message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()
