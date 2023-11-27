#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsD3D12/Device/DiligentCore.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(xiiGAL, GraphicsD3D12)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiDiligentCore::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiDiligentCore::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Startup);
