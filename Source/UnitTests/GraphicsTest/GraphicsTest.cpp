#include <GraphicsTest/GraphicsTestPCH.h>


#include <TestFramework/Utilities/TestSetup.h>

static xiiUniquePtr<xiiGPUTestingEnvironmentInterface> s_pGPUTestingEnvironment;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsTest, GPUTestingEnvironment)

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

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    if (s_pGPUTestingEnvironment)
    {
      s_pGPUTestingEnvironment->Shutdown();
    }

  #if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "D3D11";
  #elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "D3D12";
  #elif BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "Vulkan";
  #else
    constexpr const char* szDefaultGraphicsAPI = "Null";
  #endif

    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
    if (sGraphicsAPIName == "D3D11")
    {
      s_pGPUTestingEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentD3D11);

      if (s_pGPUTestingEnvironment->Initialize().Failed())
      {
        xiiLog::Error("Failed to initialize GPU testing environment for API ({}).", sGraphicsAPIName);
      }
    }
    else if (sGraphicsAPIName == "Vulkan")
    {
      s_pGPUTestingEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);

      if (s_pGPUTestingEnvironment->Initialize().Failed())
      {
        xiiLog::Error("Failed to initialize GPU testing environment for API ({}).", sGraphicsAPIName);
      }
    }
    else
    {
      xiiLog::Error("The selected graphics device {} is not yet implemented for testing.", sGraphicsAPIName);
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pGPUTestingEnvironment)
    {
      s_pGPUTestingEnvironment->Shutdown();
    }
    s_pGPUTestingEnvironment.Clear();
  }

XII_END_SUBSYSTEM_DECLARATION;


XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GraphicsTest", "Graphics Tests")
{
  xiiCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, xiiCommandLineUtils::PreferOsArgs);
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
