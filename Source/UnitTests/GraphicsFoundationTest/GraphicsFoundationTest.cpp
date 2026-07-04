/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

static xiiUniquePtr<xiiGPUTestingEnvironmentInterface> s_pGPUTestingEnvironment;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundationTest, GPUTestingEnvironment)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    if (sGraphicsAPIName.IsEqual_NoCase("Vulkan"))
    {
      s_pGPUTestingEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);

      if (s_pGPUTestingEnvironment->Initialize().Failed())
      {
        xiiLog::Error("Failed to initialize GPU testing environment for API ({}).", sGraphicsAPIName);
      }
    }
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
    if (sGraphicsAPIName.IsEqual_NoCase("D3D12"))
    {
      s_pGPUTestingEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentD3D12);

      if (s_pGPUTestingEnvironment->Initialize().Failed())
      {
        xiiLog::Error("Failed to initialize GPU testing environment for API ({}).", sGraphicsAPIName);
      }
    }
#endif

    if (!s_pGPUTestingEnvironment)
    {
      XII_REPORT_FAILURE("The selected graphics device {} is not yet implemented for testing.", sGraphicsAPIName);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pGPUTestingEnvironment.Clear();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GraphicsFoundationTest", "Graphics Foundation Tests")
{
  xiiCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, xiiCommandLineUtils::PreferOsArgs);
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
