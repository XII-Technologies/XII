/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <TestFramework/Utilities/TestSetup.h>

static xiiDynamicArray<xiiUniquePtr<xiiGPUTestingEnvironmentInterface>> s_GPUTestingEnvironments;

xiiUInt32 xiiGetGPUTestingEnvironmentCount()
{
  return s_GPUTestingEnvironments.GetCount();
}

xiiGPUTestingEnvironmentInterface* xiiGetGPUTestingEnvironment(xiiUInt32 uiIndex)
{
  XII_ASSERT_DEV(uiIndex < s_GPUTestingEnvironments.GetCount(), "GPU testing environment index is out of bounds.");
  return s_GPUTestingEnvironments[uiIndex].Borrow();
}

static xiiResult ConfigureTestDataDirectories()
{
  xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

  xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  sReadDir.PathParentDirectory();

  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Data/Base/", "Base"));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiDataDirUsage::AllowWrites));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

  sReadDir.Set(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  return XII_SUCCESS;
}

static void AddTestingEnvironment(xiiStringView sImplementationName)
{
  auto pEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironment, sImplementationName);
  if (pEnvironment->Initialize().Succeeded())
  {
    xiiLog::Info("Initialized graphics test implementation '{}'.", sImplementationName);
    s_GPUTestingEnvironments.PushBack(std::move(pEnvironment));
  }
  else
  {
    XII_REPORT_FAILURE("Failed to initialize graphics test implementation '{}'.", sImplementationName);
  }
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundationTest, GPUTestingEnvironment)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_VERIFY(ConfigureTestDataDirectories().Succeeded(), "Failed to configure the GraphicsFoundationTest data directories.");

    const bool          bRendererWasSelected = xiiCommandLineUtils::GetGlobalInstance()->HasOption("-renderer");
    const xiiStringView sSelectedRenderer    = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    bool                bRendererWasFound    = false;

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    if (!bRendererWasSelected || sSelectedRenderer.IsEqual_NoCase("Vulkan"))
    {
      bRendererWasFound = true;
      AddTestingEnvironment("Vulkan");
    }
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
    if (!bRendererWasSelected || sSelectedRenderer.IsEqual_NoCase("D3D12"))
    {
      bRendererWasFound = true;
      AddTestingEnvironment("D3D12");
    }
#endif

    if (bRendererWasSelected && !bRendererWasFound)
    {
      XII_REPORT_FAILURE("The selected graphics implementation '{}' is not compiled into GraphicsFoundationTest.", sSelectedRenderer);
    }

    XII_VERIFY(!s_GPUTestingEnvironments.IsEmpty(), "No graphics implementation could be initialized for testing.");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    for (auto& pEnvironment : s_GPUTestingEnvironments)
    {
      pEnvironment->Shutdown();
    }
    s_GPUTestingEnvironments.Clear();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GraphicsFoundationTest", "Graphics Foundation Tests")
{
  xiiCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, xiiCommandLineUtils::PreferOsArgs);
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
