/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFrameworkTest/AssetProcessorTest.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Types/ScopeExit.h>

void TestAssetProcessorCommandLine(xiiStringView projectDirectory)
{
  xiiStringBuilder project(projectDirectory), preferences(projectDirectory), profilePath(projectDirectory);
  project.AppendPath("xiiProject");
  preferences.AppendPath("Processor/CommandLinePreferences");
  profilePath.AppendPath("Editor/AssetProfiles.ddl");
  auto run = [&](xiiStringView profile, xiiStringView error, xiiInt32 expectedExitCode = 1) {
    xiiProcessOptions options;
    xiiStringBuilder  executable(xiiOSFile::GetApplicationDirectory());
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    executable.AppendPath("xiiEditorProcessor.exe");
#else
    executable.AppendPath("xiiEditorProcessor");
#endif
    options.m_sProcess = executable;
    options.AddArgument("-project");
    options.AddArgument(project);
    options.AddArgument("-transform");
    options.AddArgument(profile);
    options.AddArgument("-outputDir");
    options.AddArgument(preferences);
    options.AddArgument("-renderer");
    options.AddArgument("Vulkan");
    options.AddArgument("-norecent");
    xiiStringBuilder output;
    options.m_onStdOut = [&](xiiStringView text) {
      output.Append(text);
    };
    xiiProcess process;
    if (!XII_TEST_RESULT(process.Launch(options)) || !XII_TEST_RESULT(process.WaitToFinish(xiiTime::MakeFromSeconds(30))))
      return false;
    XII_TEST_INT(process.GetExitCode(), expectedExitCode);
    XII_TEST_BOOL_MSG(output.FindSubString(error) != nullptr, "%s", output.GetData());
    return true;
  };
  if (!run("MissingProcessorTestProfile", "Asset platform config 'MissingProcessorTestProfile' is unknown"))
    return;

  xiiContiguousMemoryStreamStorage original;
  {
    xiiFileReader file;
    if (!XII_TEST_RESULT(file.Open(profilePath)))
      return;
    original.ReadAll(file);
  }
  XII_SCOPE_EXIT(xiiDeferredFileWriter restore; restore.SetOutput(profilePath);
                 XII_TEST_RESULT(original.CopyToStream(restore)); XII_TEST_RESULT(restore.Close()));
  const xiiString invalid = "AssetProfiles {}";
  {
    xiiDeferredFileWriter file;
    file.SetOutput(profilePath);
    if (!XII_TEST_RESULT(file.WriteBytes(invalid.GetData(), invalid.GetElementCount())) || !XII_TEST_RESULT(file.Close()))
      return;
  }
  if (!run("Default", "Asset profile configuration", 2))
    return;
  xiiFileReader file;
  if (!XII_TEST_RESULT(file.Open(profilePath)))
    return;
  xiiStringBuilder actual;
  actual.ReadAll(file);
  XII_TEST_STRING(actual, invalid);
}
