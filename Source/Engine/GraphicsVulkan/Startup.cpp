#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/Configuration/Startup.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(xiiGAL, GraphicsVulkan)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Startup);
