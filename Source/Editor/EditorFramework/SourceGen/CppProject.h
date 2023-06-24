#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/SourceGen/CppSettings.h>
#include <Foundation/Communication/Event.h>

struct XII_EDITORFRAMEWORK_DLL xiiCppProject
{
  static xiiString GetTargetSourceDir(xiiStringView sProjectDirectory = {});

  static xiiString GetGeneratorFolderName(const xiiCppSettings& cfg);

  static xiiString GetCMakeGeneratorName(const xiiCppSettings& cfg);

  static xiiString GetPluginSourceDir(const xiiCppSettings& cfg, xiiStringView sProjectDirectory = {});

  static xiiString GetBuildDir(const xiiCppSettings& cfg);

  static xiiString GetSolutionPath(const xiiCppSettings& cfg);

  static xiiResult CheckCMakeCache(const xiiCppSettings& cfg);

  static bool ExistsSolution(const xiiCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static xiiResult PopulateWithDefaultSources(const xiiCppSettings& cfg);

  static xiiResult CleanBuildDir(const xiiCppSettings& cfg);

  static xiiResult RunCMake(const xiiCppSettings& cfg);

  static xiiResult RunCMakeIfNecessary(const xiiCppSettings& cfg);

  static xiiResult CompileSolution(const xiiCppSettings& cfg);

  static xiiResult BuildCodeIfNecessary(const xiiCppSettings& cfg);

  static xiiResult FindMsBuild(const xiiCppSettings& cfg);

  static void UpdatePluginConfig(const xiiCppSettings& cfg);

  /// \brief Fired when a notable change has been made.
  static xiiEvent<const xiiCppSettings&> s_ChangeEvents;
};
