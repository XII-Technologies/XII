/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>

// Only saved in editor preferences, does not have to work cross-platform
struct XII_EDITORFRAMEWORK_DLL xiiIDE
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    VisualStudioCode,
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    VisualStudio,
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    Default = VisualStudio
#else
    Default = VisualStudioCode
#endif
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiIDE);

// Only saved in editor preferences, does not have to work cross-platform
struct XII_EDITORFRAMEWORK_DLL xiiCompiler
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Clang,
#if XII_ENABLED(XII_PLATFORM_LINUX)
    Gcc,
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
    Vs2022,
    Vs2026,
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    Default = Vs2026
#else
    Default = Gcc
#endif
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiCompiler);

struct XII_EDITORFRAMEWORK_DLL xiiCompilerPreferences
{
  xiiEnum<xiiCompiler> m_Compiler;
  bool                 m_bCustomCompiler;
  xiiString            m_sCppCompiler;
  xiiString            m_sCCompiler;
  xiiString            m_sRcCompiler;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiCompilerPreferences);

struct XII_EDITORFRAMEWORK_DLL xiiCodeEditorPreferences
{
  bool      m_bIsVisualStudio;
  xiiString m_sEditorPath;
  xiiString m_sEditorArgs;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORFRAMEWORK_DLL, xiiCodeEditorPreferences);

struct XII_EDITORFRAMEWORK_DLL xiiCppProject : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCppProject, xiiPreferences);

  struct MachineSpecificCompilerPaths
  {
    xiiString            m_sNiceName;
    xiiEnum<xiiCompiler> m_Compiler;
    xiiString            m_sCCompiler;
    xiiString            m_sCppCompiler;
    bool                 m_bIsCustom;
  };

  enum class ModifyResult
  {
    FAILURE,
    NOT_MODIFIED,
    MODIFIED
  };

  xiiCppProject();
  ~xiiCppProject();

  static xiiString GetTargetSourceDir(xiiStringView sProjectDirectory = {});

  static xiiString GetGeneratorFolderName(const xiiCppSettings& cfg);

  static xiiString GetCMakeGeneratorName(const xiiCppSettings& cfg);

  static xiiString GetPluginSourceDir(const xiiCppSettings& cfg, xiiStringView sProjectDirectory = {});

  static xiiString GetBuildDir(const xiiCppSettings& cfg);

  static xiiString GetSolutionPath(const xiiCppSettings& cfg);

  static xiiStatus OpenSolution(const xiiCppSettings& cfg);

  /// Attempts to launch the configured code editor with the specified file and line number
  static xiiStatus OpenInCodeEditor(const xiiStringView& sFileName, xiiInt32 iLineNumber);

  static xiiStringView CompilerToString(xiiCompiler::Enum compiler);

  static xiiCompiler::Enum GetSdkCompiler();

  static xiiString GetSdkCompilerMajorVersion();

  static xiiStatus TestCompiler();

  static const char* GetCMakePath();

  static xiiResult CheckCMakeCache(const xiiCppSettings& cfg);

  static ModifyResult CheckCMakeUserPresets(const xiiCppSettings& cfg, bool bWriteResult);

  static bool ExistsSolution(const xiiCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static xiiResult PopulateWithDefaultSources(const xiiCppSettings& cfg);

  static xiiResult CleanBuildDir(const xiiCppSettings& cfg);

  static xiiResult RunCMake(const xiiCppSettings& cfg);

  static xiiResult RunCMakeIfNecessary(const xiiCppSettings& cfg);

  static xiiResult CompileSolution(const xiiCppSettings& cfg);

  static xiiResult BuildCodeIfNecessary(const xiiCppSettings& cfg);

  static xiiVariantDictionary CreateEmptyCMakeUserPresetsJson(const xiiCppSettings& cfg);

  static ModifyResult ModifyCMakeUserPresetsJson(const xiiCppSettings& cfg, xiiVariantDictionary& inout_json);

  static void UpdatePluginConfig(const xiiCppSettings& cfg);

  static xiiResult EnsureCppPluginReady();

  static bool IsBuildRequired();

  /// Fired when a notable change has been made.
  static xiiEvent<const xiiCppSettings&> s_ChangeEvents;

  static void LoadPreferences();

  static xiiArrayPtr<const MachineSpecificCompilerPaths> GetMachineSpecificCompilers() { return s_MachineSpecificCompilers.GetArrayPtr(); }

  // Change the current preferences to point to a SDK compatible compiler
  static xiiResult ForceSdkCompatibleCompiler();

private:
  xiiEnum<xiiIDE>          m_Ide;
  xiiCompilerPreferences   m_CompilerPreferences;
  xiiCodeEditorPreferences m_CodeEditorPreferences;

  static xiiDynamicArray<MachineSpecificCompilerPaths> s_MachineSpecificCompilers;
};
