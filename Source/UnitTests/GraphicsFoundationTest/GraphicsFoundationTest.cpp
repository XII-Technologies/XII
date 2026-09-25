/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <TestFramework/Utilities/TestSetup.h>

static xiiHybridArray<xiiString, 2U> s_GPUTestingEnvironmentNames;
static bool                           s_bTestDataDirectoriesConfigured = false;

xiiUInt32 xiiGetGPUTestingEnvironmentCount()
{
  return s_GPUTestingEnvironmentNames.GetCount();
}

xiiStringView xiiGetGPUTestingEnvironmentName(xiiUInt32 uiIndex)
{
  XII_ASSERT_DEV(uiIndex < s_GPUTestingEnvironmentNames.GetCount(), "GPU testing environment index is out of bounds.");
  return s_GPUTestingEnvironmentNames[uiIndex];
}

xiiResult xiiConfigureGPUTestDataDirectories()
{
  if (s_bTestDataDirectoriesConfigured)
    return XII_SUCCESS;

  xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

  xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  sReadDir.PathParentDirectory();

  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Data/Base/", "Base"));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiDataDirUsage::AllowWrites));
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

  sReadDir.Set(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  s_bTestDataDirectoriesConfigured = true;
  return XII_SUCCESS;
}

static void SelectTestingEnvironments()
{
  const bool          bRendererWasSelected = xiiCommandLineUtils::GetGlobalInstance()->HasOption("-renderer");
  const xiiStringView sSelectedRenderer    = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
  bool                bRendererWasFound    = false;

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  if (!bRendererWasSelected || sSelectedRenderer.IsEqual_NoCase("Vulkan"))
  {
    bRendererWasFound = true;
    s_GPUTestingEnvironmentNames.PushBack("Vulkan");
  }
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
  if (!bRendererWasSelected || sSelectedRenderer.IsEqual_NoCase("D3D12"))
  {
    bRendererWasFound = true;
    s_GPUTestingEnvironmentNames.PushBack("D3D12");
  }
#endif

  if (bRendererWasSelected && !bRendererWasFound)
  {
    XII_REPORT_FAILURE("The selected graphics implementation '{}' is not compiled into GraphicsFoundationTest.", sSelectedRenderer);
  }

  XII_VERIFY(!s_GPUTestingEnvironmentNames.IsEmpty(), "No graphics implementation is compiled into GraphicsFoundationTest.");
}

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GraphicsFoundationTest", "Graphics Foundation Tests")
{
  xiiCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, xiiCommandLineUtils::PreferOsArgs);
  SelectTestingEnvironments();
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
