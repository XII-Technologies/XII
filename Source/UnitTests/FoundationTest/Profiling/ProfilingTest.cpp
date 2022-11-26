#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  void WriteOutProfilingCapture(const char* szFilePath)
  {
    xiiStringBuilder outputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", xiiFileSystem::AllowWrites) == XII_SUCCESS);

    xiiFileWriter fileWriter;
    if (fileWriter.Open(szFilePath) == XII_SUCCESS)
    {
      xiiProfilingSystem::ProfilingData profilingData;
      xiiProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      xiiLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(Profiling);

XII_CREATE_SIMPLE_TEST(Profiling, Profiling)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested scopes")
  {
    xiiProfilingSystem::Clear();

    {
      XII_PROFILE_SCOPE("Prewarm scope");
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(1));
    }

    xiiTime endTime = xiiTime::Now() + xiiTime::Milliseconds(1);

    {
      XII_PROFILE_SCOPE("Outer scope");

      {
        XII_PROFILE_SCOPE("Inner scope");

        while (xiiTime::Now() < endTime)
        {
        }
      }
    }

    WriteOutProfilingCapture(":output/profilingScopes.json");
  }
}
