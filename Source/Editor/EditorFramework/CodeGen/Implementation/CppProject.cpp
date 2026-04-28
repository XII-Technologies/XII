/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Shlobj.h>
#endif

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiIDE, 1)
  XII_ENUM_CONSTANT(xiiIDE::VisualStudioCode),
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  XII_ENUM_CONSTANT(xiiIDE::VisualStudio),
#endif
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCompiler, 1)
  XII_ENUM_CONSTANT(xiiCompiler::Clang),
#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ENUM_CONSTANT(xiiCompiler::Gcc),
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
  XII_ENUM_CONSTANT(xiiCompiler::Vs2026),
  XII_ENUM_CONSTANT(xiiCompiler::Vs2022),
#endif
XII_END_STATIC_REFLECTED_ENUM;

#if XII_ENABLED(XII_PLATFORM_LINUX)
#define CPP_COMPILER_DEFAULT "g++"
#define C_COMPILER_DEFAULT "gcc"
#elif XII_ENABLED(XII_PLATFORM_WINDOWS)
#define CPP_COMPILER_DEFAULT ""
#define C_COMPILER_DEFAULT ""
#else
#error Platform not implemented
#endif

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiCompilerPreferences, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiCompilerPreferences>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Compiler", xiiCompiler, m_Compiler)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("CustomCompiler", m_bCustomCompiler)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("CppCompiler", m_sCppCompiler)->AddAttributes(new xiiDefaultValueAttribute(CPP_COMPILER_DEFAULT)),
    XII_MEMBER_PROPERTY("CCompiler", m_sCCompiler)->AddAttributes(new xiiDefaultValueAttribute(C_COMPILER_DEFAULT)),
    XII_MEMBER_PROPERTY("RcCompiler", m_sRcCompiler),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiCodeEditorPreferences, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiCodeEditorPreferences>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CodeEditorPath", m_sEditorPath)->AddAttributes(new xiiExternalFileBrowserAttribute("Select Editor", "*.exe"_xiisv)),
    XII_MEMBER_PROPERTY("CodeEditorArgs", m_sEditorArgs)->AddAttributes(new xiiDefaultValueAttribute("{file} {line}")),
    XII_MEMBER_PROPERTY("IsVisualStudio", m_bIsVisualStudio)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCppProject, 1, xiiRTTIDefaultAllocator<xiiCppProject>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("CppIDE", xiiIDE, m_Ide),
    XII_MEMBER_PROPERTY("CompilerPreferences", m_CompilerPreferences),
    XII_MEMBER_PROPERTY("CodeEditorPreferences", m_CodeEditorPreferences),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEvent<const xiiCppSettings&> xiiCppProject::s_ChangeEvents;

xiiDynamicArray<xiiCppProject::MachineSpecificCompilerPaths> xiiCppProject::s_MachineSpecificCompilers;

namespace
{
  static constexpr xiiUInt32 minGccVersion   = 10;
  static constexpr xiiUInt32 maxGccVersion   = 20;
  static constexpr xiiUInt32 minClangVersion = 10;
  static constexpr xiiUInt32 maxClangVersion = 20;

  xiiResult TestCompilerExecutable(xiiStringView sName, xiiString* out_pVersion = nullptr)
  {
    xiiStringBuilder  sStdout;
    xiiProcessOptions po;
    po.AddArgument("--version");
    po.m_sProcess = sName;
    po.m_onStdOut = [&sStdout](xiiStringView out) {
      sStdout.Append(out);
    };

    if (xiiProcess::Execute(po).Failed())
      return XII_FAILURE;

    xiiHybridArray<xiiStringView, 8> lines;
    sStdout.Split(false, lines, "\r", "\n");
    if (lines.IsEmpty())
      return XII_FAILURE;

    xiiHybridArray<xiiStringView, 4> splitResult;
    lines[0].Split(false, splitResult, " ");

    if (splitResult.IsEmpty())
      return XII_FAILURE;

    xiiStringView version = splitResult.PeekBack();
    splitResult.Clear();
    version.Split(false, splitResult, ".");
    if (splitResult.GetCount() < 3)
    {
      return XII_FAILURE;
    }

    if (out_pVersion)
    {
      *out_pVersion = version;
    }

    return XII_SUCCESS;
  }

  void AddCompilerVersions(xiiDynamicArray<xiiCppProject::MachineSpecificCompilerPaths>& inout_compilers, xiiCompiler::Enum compiler, xiiStringView sRequiredMajorVersion)
  {
    xiiStringView compilerBaseName;
    xiiStringView compilerBaseNameCpp;
    switch (compiler)
    {
      case xiiCompiler::Clang:
        compilerBaseName    = "clang";
        compilerBaseNameCpp = "clang++";
        break;
#if XII_ENABLED(XII_PLATFORM_LINUX)
      case xiiCompiler::Gcc:
        compilerBaseName    = "gcc";
        compilerBaseNameCpp = "g++";
        break;
#endif

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }


    xiiString        compilerVersion;
    xiiStringBuilder requiredVersion = sRequiredMajorVersion;
    requiredVersion.Append('.');
    xiiStringBuilder fmt;
    if (TestCompilerExecutable(compilerBaseName, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerBaseNameCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (system default = {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerBaseName, compilerBaseNameCpp, false});
    }

    xiiStringBuilder compilerExecutable;
    xiiStringBuilder compilerExecutableCpp;
    compilerExecutable.SetFormat("{}-{}", compilerBaseName, sRequiredMajorVersion);
    compilerExecutableCpp.SetFormat("{}-{}", compilerBaseNameCpp, sRequiredMajorVersion);
    if (TestCompilerExecutable(compilerExecutable, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerExecutableCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (version {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerExecutable, compilerExecutableCpp, false});
    }
  }
} // namespace

xiiString xiiCppProject::GetTargetSourceDir(xiiStringView sProjectDirectory /*= {}*/)
{
  xiiStringBuilder sTargetDir = sProjectDirectory;

  if (sTargetDir.IsEmpty())
  {
    sTargetDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  }

  sTargetDir.AppendPath("Source");
  return sTargetDir;
}

xiiString xiiCppProject::GetGeneratorFolderName(const xiiCppSettings& cfg)
{
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  switch (pProjectPreferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    case xiiCompiler::Vs2026:
      return "Vs2026x64";
    case xiiCompiler::Vs2022:
      return "Vs2022x64";
#endif
    case xiiCompiler::Clang:
      return "Clangx64";
#if XII_ENABLED(XII_PLATFORM_LINUX)
    case xiiCompiler::Gcc:
      return "Gccx64";
#endif
  }
  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

xiiString xiiCppProject::GetCMakeGeneratorName(const xiiCppSettings& cfg)
{

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  switch (pProjectPreferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
    case xiiCompiler::Vs2026:
      return "Visual Studio 18 2026";
    case xiiCompiler::Vs2022:
      return "Visual Studio 17 2022";
    case xiiCompiler::Clang:
      return "Ninja";
  }
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  return "Ninja";
#else
#  error Platform not implemented
#endif
  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

xiiString xiiCppProject::GetPluginSourceDir(const xiiCppSettings& cfg, xiiStringView sProjectDirectory /*= {}*/)
{
  xiiStringBuilder sDir = GetTargetSourceDir(sProjectDirectory);
  sDir.AppendPath(cfg.m_sPluginName);
  sDir.Append("Plugin");
  return sDir;
}

xiiString xiiCppProject::GetBuildDir(const xiiCppSettings& cfg)
{
  xiiStringBuilder sBuildDir;
  sBuildDir.SetFormat("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

xiiString xiiCppProject::GetSolutionPath(const xiiCppSettings& cfg)
{
  xiiStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();
  if (pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2026)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".slnx");
    return sSolutionFile;
  }
  else if (pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2022)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".sln");
    return sSolutionFile;
  }
#endif

  sSolutionFile.AppendPath("build.ninja");
  return sSolutionFile;
}

xiiStatus xiiCppProject::OpenSolution(const xiiCppSettings& cfg)
{
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  switch (pProjectPreferences->m_Ide.GetValue())
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    case xiiIDE::VisualStudio:
    {
      if (!xiiQtUiServices::OpenFileInDefaultProgram(xiiCppProject::GetSolutionPath(cfg)))
      {
        return xiiStatus("Opening the solution in Visual Studio failed.");
      }
    }
    break;
#endif
    case xiiIDE::VisualStudioCode:
    {
      auto        solutionPath = xiiCppProject::GetTargetSourceDir();
      QStringList args;
      args.push_back(QString::fromUtf8(solutionPath.GetData(), solutionPath.GetElementCount()));
      if (xiiStatus status = xiiQtUiServices::OpenInVsCode(args); status.Failed())
      {
        return xiiStatus(xiiFmt("Opening Visual Studio Code failed: {}", status.GetMessageString()));
      }
    }
    break;
  }

  return XII_SUCCESS;
}

xiiStatus xiiCppProject::OpenInCodeEditor(const xiiStringView& sFileName, xiiInt32 iLineNumber)
{
  if (!xiiOSFile::ExistsFile(sFileName))
  {
    return xiiStatus("Failed finding filename");
  }

  xiiStringBuilder sLineNumber;
  xiiConversionUtils::ToString(iLineNumber, sLineNumber);

  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  // Visual Studio does not expose a CLI command to open a file/line in all use-cases directly
  // therefore run a custom .vbs script which controls VS and performs the needed actions for us. This avoids pulling COM interfacing into the project.
  if (pProjectPreferences->m_CodeEditorPreferences.m_bIsVisualStudio)
  {
    xiiStringBuilder dir;
    if (xiiFileSystem::ResolveSpecialDirectory(">sdk/Utilities/Scripts/open-in-msvs.vbs", dir).Failed())
    {
      return xiiStatus("Failed resolving path to \">sdk/Utilities/Scripts/open-in-msvs.vbs\"");
    }

    if (!xiiOSFile::ExistsFile(dir))
    {
      return xiiStatus(xiiFmt("File does not exist '{0}'", dir));
    }

    QStringList args;
    args.append("/B");
    args.append(QString::fromUtf8(dir.GetData()));
    args.append(QString::fromUtf8(sFileName.GetStartPointer(), sFileName.GetElementCount()));
    args.append(QString::fromUtf8(sLineNumber.GetData()));

    QProcess proc;
    if (proc.startDetached("cscript", args) == false)
    {
      return xiiStatus("Failed to launch code editor");
    }

    return XII_SUCCESS;
  }

  xiiStringBuilder sFormatString = pProjectPreferences->m_CodeEditorPreferences.m_sEditorArgs;
  if (sFormatString.IsEmpty())
  {
    return xiiStatus("Code editor is not configured");
  }

  sFormatString.ReplaceAll("{line}", sLineNumber);
  sFormatString.ReplaceAll("{file}", sFileName);

  const QStringList args         = QProcess::splitCommand(QString::fromUtf8(sFormatString.GetData()));
  const QString     sProgramPath = QString::fromUtf8(pProjectPreferences->m_CodeEditorPreferences.m_sEditorPath.GetData());

  QProcess proc;
  if (proc.startDetached(sProgramPath, args) == false)
  {
    return xiiStatus("Failed to launch code editor");
  }
  return XII_SUCCESS;
}

xiiStringView xiiCppProject::CompilerToString(xiiCompiler::Enum compiler)
{
  switch (compiler)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    case xiiCompiler::Vs2026:
      return "Vs2026";
    case xiiCompiler::Vs2022:
      return "Vs2022";
#endif
    case xiiCompiler::Clang:
      return "Clang";
#if XII_ENABLED(XII_PLATFORM_LINUX)
    case xiiCompiler::Gcc:
      return "Gcc";
#endif
    default:
      break;
  }
  XII_ASSERT_NOT_IMPLEMENTED;
  return "<not implemented>";
}

xiiCompiler::Enum xiiCppProject::GetSdkCompiler()
{
#if XII_ENABLED(XII_COMPILER_CLANG)
  return xiiCompiler::Clang;
#elif XII_ENABLED(XII_COMPILER_GCC)
  return xiiCompiler::Gcc;
#elif XII_ENABLED(XII_COMPILER_MSVC)
#  if _MSC_VER >= 1950
  return xiiCompiler::Vs2026;
#  else
  return xiiCompiler::Vs2022;
#  endif
#else
#  error Unknown compiler
#endif
}

xiiString xiiCppProject::GetSdkCompilerMajorVersion()
{
#if XII_ENABLED(XII_COMPILER_MSVC)
  xiiStringBuilder fmt;
  fmt.SetFormat("{}.{}", _MSC_VER / 100, _MSC_VER % 100);
  return fmt;
#elif XII_ENABLED(XII_COMPILER_CLANG)
  return XII_PP_STRINGIFY(__clang_major__);
#elif XII_ENABLED(XII_COMPILER_GCC)
  return XII_PP_STRINGIFY(__GNUC__);
#else
#  error Unsupported compiler
#endif
}

xiiStatus xiiCppProject::TestCompiler()
{
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();
  if (pProjectPreferences->m_CompilerPreferences.m_Compiler != GetSdkCompiler())
  {
    return xiiStatus(xiiFmt("The currently configured compiler is incompatible with this SDK. The SDK was built with '{}' but the currently configured compiler is '{}'.", CompilerToString(GetSdkCompiler()), CompilerToString(pProjectPreferences->m_CompilerPreferences.m_Compiler)));
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  // As CMake is selecting the compiler it is hard to do a version check, for now just assume they are compatible.
  if (GetSdkCompiler() == xiiCompiler::Vs2026 || GetSdkCompiler() == xiiCompiler::Vs2022)
    return XII_SUCCESS;

  if (GetSdkCompiler() == xiiCompiler::Clang)
  {
    if (!xiiOSFile::ExistsFile(pProjectPreferences->m_CompilerPreferences.m_sRcCompiler))
    {
      return xiiStatus(xiiFmt("The selected RC compiler '{}' does not exist on disk.", pProjectPreferences->m_CompilerPreferences.m_sRcCompiler));
    }
  }
#endif

  xiiString cCompilerVersion, cppCompilerVersion;
  if (TestCompilerExecutable(pProjectPreferences->m_CompilerPreferences.m_sCCompiler, &cCompilerVersion).Failed())
  {
    return xiiStatus("The selected C Compiler doesn't work or doesn't exist.");
  }
  if (TestCompilerExecutable(pProjectPreferences->m_CompilerPreferences.m_sCppCompiler, &cppCompilerVersion).Failed())
  {
    return xiiStatus("The selected C++ Compiler doesn't work or doesn't exist.");
  }

  xiiStringBuilder sdkCompilerMajorVersion = GetSdkCompilerMajorVersion();
  sdkCompilerMajorVersion.Append('.');
  if (!cCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return xiiStatus(xiiFmt("The selected C Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cCompilerVersion));
  }
  if (!cppCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return xiiStatus(xiiFmt("The selected C++ Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cppCompilerVersion));
  }

  return XII_SUCCESS;
}

const char* xiiCppProject::GetCMakePath()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  return "cmake/bin/cmake";
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  return "cmake";
#else
#  error Platform not implemented
#endif
}

xiiResult xiiCppProject::CheckCMakeCache(const xiiCppSettings& cfg)
{
  xiiStringBuilder sCacheFile;
  sCacheFile = GetBuildDir(cfg);
  sCacheFile.AppendPath("CMakeCache.txt");

  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sCacheFile));

  xiiStringBuilder content;
  content.ReadAll(file);

  const xiiStringView sSearchFor = "CMAKE_CONFIGURATION_TYPES:STRING="_xiisv;

  const char* pConfig = content.FindSubString(sSearchFor);
  if (pConfig == nullptr)
    return XII_FAILURE;

  pConfig += sSearchFor.GetElementCount();

  const char* pEndConfig = content.FindSubString("\n", pConfig);
  if (pEndConfig == nullptr)
    return XII_FAILURE;

  xiiStringBuilder sUsedCfg;
  sUsedCfg.SetSubString_FromTo(pConfig, pEndConfig);
  sUsedCfg.Trim("\t\n\r ");

  if (sUsedCfg != BUILDSYSTEM_BUILDTYPE)
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiCppProject::ModifyResult xiiCppProject::CheckCMakeUserPresets(const xiiCppSettings& cfg, bool bWriteResult)
{
  xiiStringBuilder configureJsonPath = xiiCppProject::GetPluginSourceDir(cfg).GetFileDirectory();
  configureJsonPath.AppendPath("CMakeUserPresets.json");

  if (xiiOSFile::ExistsFile(configureJsonPath))
  {
    xiiFileReader fileReader;
    if (fileReader.Open(configureJsonPath).Failed())
    {
      xiiLog::Error("Failed to open '{}' for reading", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    xiiJSONReader reader;
    if (reader.Parse(fileReader).Failed())
    {
      xiiLog::Error("Failed to parse JSON of '{}'", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    fileReader.Close();

    if (reader.GetTopLevelElementType() != xiiJSONReader::ElementType::Dictionary)
    {
      xiiLog::Error("Top level element of '{}' is expected to be a dictionary. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    xiiVariantDictionary json         = reader.GetTopLevelObject();
    auto                 modifyResult = ModifyCMakeUserPresetsJson(cfg, json);
    if (modifyResult == ModifyResult::FAILURE)
    {
      xiiLog::Error("Failed to modify '{}' in place. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    if (bWriteResult && modifyResult == ModifyResult::MODIFIED)
    {
      xiiStandardJSONWriter jsonWriter;
      xiiDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(xiiVariant(json));
      if (fileWriter.Close().Failed())
      {
        xiiLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }

    return modifyResult;
  }
  else
  {
    if (bWriteResult)
    {
      xiiStandardJSONWriter jsonWriter;
      xiiDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(xiiVariant(CreateEmptyCMakeUserPresetsJson(cfg)));
      if (fileWriter.Close().Failed())
      {
        xiiLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }
  }

  return ModifyResult::MODIFIED;
}

bool xiiCppProject::ExistsSolution(const xiiCppSettings& cfg)
{
  return xiiOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool xiiCppProject::ExistsProjectCMakeListsTxt()
{
  if (!xiiToolsProject::IsProjectOpen())
    return false;

  xiiStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return xiiOSFile::ExistsFile(sPath);
}

xiiResult xiiCppProject::PopulateWithDefaultSources(const xiiCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const xiiString sProjectName = cfg.m_sPluginName;

  xiiStringBuilder sProjectNameUpper = cfg.m_sPluginName;
  sProjectNameUpper.ToUpper();

  const xiiStringBuilder sTargetDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();

  xiiStringBuilder sSourceDir = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("SourceTemplate");

  xiiDynamicArray<xiiFileStats> items;
  xiiOSFile::GatherAllItemsInFolder(items, sSourceDir, xiiFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    xiiString m_sSource;
    xiiString m_sDestination;
  };

  xiiHybridArray<FileToCopy, 32> filesCopied;

  // Gather files
  {
    for (const auto& item : items)
    {
      xiiStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("SourceTemplate", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (xiiOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        filesCopied.Clear();
        break;
      }

      auto& ftc          = filesCopied.ExpandAndGetRef();
      ftc.m_sSource      = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  // Copy files
  {
    for (const auto& ftc : filesCopied)
    {
      if (xiiOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        xiiLog::Error("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return XII_FAILURE;
      }
    }
  }

  // Modify sources
  {
    for (const auto& filePath : filesCopied)
    {
      xiiStringBuilder content;

      {
        xiiFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          xiiLog::Error("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return XII_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("SourceTemplate", sProjectName);
      content.ReplaceAll("SOURCETEMPLATE", sProjectNameUpper);

      {
        xiiFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          xiiLog::Error("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return XII_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  s_ChangeEvents.Broadcast(cfg);
  return XII_SUCCESS;
}

xiiResult xiiCppProject::CleanBuildDir(const xiiCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const xiiString sBuildDir = GetBuildDir(cfg);

  if (!xiiOSFile::ExistsDirectory(sBuildDir))
    return XII_SUCCESS;

  return xiiOSFile::DeleteFolder(sBuildDir);
}

xiiResult xiiCppProject::RunCMake(const xiiCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  if (!ExistsProjectCMakeListsTxt())
  {
    xiiLog::Error("No CMakeLists.txt exists in target source directory '{}'", GetTargetSourceDir());
    return XII_FAILURE;
  }

  if (auto compilerWorking = TestCompiler(); compilerWorking.Failed())
  {
    compilerWorking.LogFailure();
    return XII_FAILURE;
  }

  if (CheckCMakeUserPresets(cfg, true) == ModifyResult::FAILURE)
  {
    return XII_FAILURE;
  }

  xiiStringBuilder tmp;

  QStringList args;
  args << "--preset";
  args << "xiiEngine";

  xiiLogSystemToBuffer log;


  const xiiString sTargetSourceDir = xiiCppProject::GetTargetSourceDir();

  xiiStatus res = xiiQtEditorApp::GetSingleton()->ExecuteTool(GetCMakePath(), args, 120, &log, xiiLogMsgType::InfoMsg, sTargetSourceDir);

  if (res.Failed())
  {
    xiiLog::Error("CMake generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.GetMessageString());
    return XII_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    xiiLog::Error("CMake did not generate the expected output. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return XII_FAILURE;
  }

  xiiLog::Success("CMake generation successful.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return XII_SUCCESS;
}

xiiResult xiiCppProject::RunCMakeIfNecessary(const xiiCppSettings& cfg)
{
  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
    return XII_SUCCESS;

  auto userPresetResult = CheckCMakeUserPresets(cfg, false);
  if (userPresetResult == ModifyResult::FAILURE)
    return XII_FAILURE;

  if (xiiCppProject::ExistsSolution(cfg) && xiiCppProject::CheckCMakeCache(cfg).Succeeded() && userPresetResult == ModifyResult::NOT_MODIFIED)
    return XII_SUCCESS;

  return xiiCppProject::RunCMake(cfg);
}

xiiResult xiiCppProject::CompileSolution(const xiiCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  XII_LOG_BLOCK("Compile C++ Plugin");

  xiiHybridArray<xiiString, 32> errors;
  xiiInt32                      iReturnCode = 0;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (xiiSystemInformation::IsDebuggerAttached())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  xiiProcessOptions po;

  xiiString cmakePath = xiiQtEditorApp::GetSingleton()->FindToolApplication(xiiCppProject::GetCMakePath());

  po.m_sProcess = cmakePath;
  po.AddArgument("--build");
  po.AddArgument(GetBuildDir(cfg));
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  if (pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2026 || pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2022)
  {
    po.AddArgument("--config");
    po.AddArgument(BUILDSYSTEM_BUILDTYPE);
  }
#endif
  po.m_sWorkingDirectory  = GetBuildDir(cfg);
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut           = [&](xiiStringView sText) {
    if (sText.FindSubString_NoCase("error") != nullptr)
    {
      errors.PushBack(sText);
    }
  };
  po.m_onStdError = [&](xiiStringView sText) {
    if (sText.FindSubString_NoCase("error") != nullptr)
    {
      errors.PushBack(sText);
    }
  };

  xiiStringBuilder sCMakeBuildCmd;
  po.BuildCommandLineString(sCMakeBuildCmd);
  xiiLog::Dev("Running {} {}", cmakePath, sCMakeBuildCmd);
  if (xiiProcess::Execute(po, &iReturnCode).Failed())
  {
    xiiLog::Error("Failed to start CMake.");
    return XII_FAILURE;
  }

  if (iReturnCode == 0)
  {
    xiiLog::Success("Compiled C++ code.");
    return XII_SUCCESS;
  }

  xiiLog::Error("CMake --build failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    xiiLog::Error(err);
  }

  return XII_FAILURE;
}

xiiResult xiiCppProject::BuildCodeIfNecessary(const xiiCppSettings& cfg)
{
  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
    return XII_SUCCESS;

  if (!xiiCppProject::ExistsSolution(cfg) || xiiCppProject::CheckCMakeCache(cfg).Failed())
  {
    XII_SUCCEED_OR_RETURN(xiiCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

xiiVariantDictionary xiiCppProject::CreateEmptyCMakeUserPresetsJson(const xiiCppSettings& cfg)
{
  xiiVariantDictionary json;
  json.Insert("version", 3);

  {
    xiiVariantDictionary cmakeMinimumRequired;
    cmakeMinimumRequired.Insert("major", 3);
    cmakeMinimumRequired.Insert("minor", 21);
    cmakeMinimumRequired.Insert("patch", 0);

    json.Insert("cmakeMinimumRequired", std::move(cmakeMinimumRequired));
  }

  {
    xiiVariantArray      configurePresets;
    xiiVariantDictionary xiiEnginePreset;
    xiiEnginePreset.Insert("name", "xiiEngine");
    xiiEnginePreset.Insert("displayName", "Build the xiiEngine Plugin");

    {
      xiiVariantDictionary cacheVariables;
      xiiEnginePreset.Insert("cacheVariables", std::move(cacheVariables));
    }

    configurePresets.PushBack(std::move(xiiEnginePreset));
    json.Insert("configurePresets", std::move(configurePresets));
  }

  {
    xiiVariantArray buildPresets;
    {
      xiiVariantDictionary xiiEngineBuildPreset;
      xiiEngineBuildPreset.Insert("name", "xiiEngine");
      xiiEngineBuildPreset.Insert("configurePreset", "xiiEngine");
      buildPresets.PushBack(std::move(xiiEngineBuildPreset));
    }
    json.Insert("buildPresets", std::move(buildPresets));
  }

  XII_VERIFY(ModifyCMakeUserPresetsJson(cfg, json) == ModifyResult::MODIFIED, "Freshly created user presets file should always be modified");

  return json;
}

void xiiCppProject::UpdatePluginConfig(const xiiCppSettings& cfg)
{
  const xiiStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  xiiPluginBundleSet& bundles = xiiQtEditorApp::GetSingleton()->GetPluginBundles();

  xiiStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  xiiPluginBundle& plugin       = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy            = true;
  plugin.m_bSelected            = true;
  plugin.m_bMissing             = true;
  plugin.m_LastModificationTime = xiiTimestamp::MakeInvalid();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  xiiQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}

xiiResult xiiCppProject::EnsureCppPluginReady()
{
  if (!ExistsProjectCMakeListsTxt())
    return XII_SUCCESS;

  xiiCppSettings cppSettings;
  if (cppSettings.Load().Failed())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning(xiiFmt("Failed to load the C++ plugin settings."));
    return XII_FAILURE;
  }

  if (xiiCppProject::BuildCodeIfNecessary(cppSettings).Failed())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning(xiiFmt("Failed to build the C++ code. See log for details."));
    return XII_FAILURE;
  }

  xiiQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  return XII_SUCCESS;
}

bool xiiCppProject::IsBuildRequired()
{
  if (!ExistsProjectCMakeListsTxt())
    return false;

  xiiCppSettings cfg;
  if (cfg.Load().Failed())
    return false;

  if (!xiiCppProject::ExistsSolution(cfg))
    return true;

  if (xiiCppProject::CheckCMakeCache(cfg).Failed())
    return true;

  xiiStringBuilder sPath = xiiOSFile::GetApplicationDirectory();
  sPath.AppendPath(cfg.m_sPluginName);

  sPath.Append("Plugin");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  if (!xiiOSFile::ExistsFile(sPath))
    return true;

  return false;
}

namespace
{
  template <typename T>
  T* Expect(xiiVariantDictionary& inout_json, xiiStringView sName)
  {
    xiiVariant* var = nullptr;
    if (inout_json.TryGetValue(sName, var) && var->IsA<T>())
    {
      return &var->GetWritable<T>();
    }
    return nullptr;
  }

  void Modify(xiiVariantDictionary& inout_json, xiiStringView sName, xiiStringView sValue, xiiCppProject::ModifyResult& inout_modified)
  {
    xiiVariant* currentValue = nullptr;
    if (inout_json.TryGetValue(sName, currentValue) && currentValue->IsA<xiiString>() && currentValue->Get<xiiString>() == sValue)
      return;

    inout_json[sName] = sValue;
    inout_modified    = xiiCppProject::ModifyResult::MODIFIED;
  }

  void Remove(xiiVariantDictionary& inout_json, xiiStringView sName, xiiCppProject::ModifyResult& inout_modified)
  {
    if (inout_json.Remove(sName))
    {
      inout_modified = xiiCppProject::ModifyResult::MODIFIED;
    }
  }

} // namespace

xiiCppProject::ModifyResult xiiCppProject::ModifyCMakeUserPresetsJson(const xiiCppSettings& cfg, xiiVariantDictionary& inout_json)
{
  auto result           = ModifyResult::NOT_MODIFIED;
  auto configurePresets = Expect<xiiVariantArray>(inout_json, "configurePresets");
  if (!configurePresets)
    return ModifyResult::FAILURE;

  const xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  for (auto& preset : *configurePresets)
  {
    if (!preset.IsA<xiiVariantDictionary>())
      continue;

    auto& presetDict = preset.GetWritable<xiiVariantDictionary>();

    auto name = Expect<xiiString>(presetDict, "name");
    if (!name || *name != "xiiEngine")
    {
      continue;
    }

    auto cacheVariables = Expect<xiiVariantDictionary>(presetDict, "cacheVariables");
    if (!cacheVariables)
      return ModifyResult::FAILURE;

    Modify(*cacheVariables, "XII_SDK_DIR", xiiFileSystem::GetSdkRootDirectory(), result);
    Modify(*cacheVariables, "XII_BUILDTYPE_ONLY", BUILDSYSTEM_BUILDTYPE, result);
    Modify(*cacheVariables, "CMAKE_BUILD_TYPE", BUILDSYSTEM_BUILDTYPE, result);

    bool bNeedsCompilerPaths = true;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    if (pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2026 || pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2022)
    {
      bNeedsCompilerPaths = false;
    }
#endif

    if (bNeedsCompilerPaths)
    {
      Modify(*cacheVariables, "CMAKE_C_COMPILER", pProjectPreferences->m_CompilerPreferences.m_sCCompiler, result);
      Modify(*cacheVariables, "CMAKE_CXX_COMPILER", pProjectPreferences->m_CompilerPreferences.m_sCppCompiler, result);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      Modify(*cacheVariables, "CMAKE_RC_COMPILER", pProjectPreferences->m_CompilerPreferences.m_sRcCompiler, result);
      Modify(*cacheVariables, "CMAKE_RC_COMPILER_INIT", "rc", result);
#endif
    }
    else
    {
      cacheVariables->Remove("CMAKE_C_COMPILER");
      cacheVariables->Remove("CMAKE_CXX_COMPILER");
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      cacheVariables->Remove("CMAKE_RC_COMPILER");
      cacheVariables->Remove("CMAKE_RC_COMPILER_INIT");
#endif
    }

    Modify(presetDict, "generator", GetCMakeGeneratorName(cfg), result);
    Modify(presetDict, "binaryDir", GetBuildDir(cfg), result);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    if (pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2026 || pProjectPreferences->m_CompilerPreferences.m_Compiler == xiiCompiler::Vs2022)
    {
      Modify(presetDict, "architecture", "x64", result);
    }
    else
#endif
    {
      Remove(presetDict, "architecture", result);
    }
  }

  return result;
}

xiiCppProject::xiiCppProject() :
  xiiPreferences(xiiPreferences::Domain::Application, "C++ Projects")
{
}
void xiiCppProject::LoadPreferences()
{
  XII_PROFILE_SCOPE("Preferences");
  auto pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  xiiCompiler::Enum sdkCompiler = GetSdkCompiler();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

  if (sdkCompiler == xiiCompiler::Vs2026)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2026 (system default)", xiiCompiler::Vs2026, "", "", false});
  }
  if (sdkCompiler == xiiCompiler::Vs2022)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2022 (system default)", xiiCompiler::Vs2022, "", "", false});
  }

#  if XII_ENABLED(XII_COMPILER_CLANG)
  // if the rcCompiler path is empty or points to a non existant file, try to autodetect it
  if ((pProjectPreferences->m_CompilerPreferences.m_sRcCompiler.IsEmpty() || !xiiOSFile::ExistsFile(pProjectPreferences->m_CompilerPreferences.m_sRcCompiler)))
  {
    xiiStringBuilder rcPath;
    HKEY             hInstalledRoots = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hInstalledRoots) == ERROR_SUCCESS)
    {
      XII_SCOPE_EXIT(RegCloseKey(hInstalledRoots));
      DWORD                    pathLengthInBytes = 0;
      xiiDynamicArray<wchar_t> path;
      if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, nullptr, &pathLengthInBytes) == ERROR_SUCCESS)
      {
        path.SetCount(pathLengthInBytes / sizeof(wchar_t));
        if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, path.GetData(), &pathLengthInBytes) == ERROR_SUCCESS)
        {
          xiiStringBuilder windowsSdkBinPath;
          windowsSdkBinPath = xiiStringWChar(path.GetData());
          windowsSdkBinPath.MakeCleanPath();
          windowsSdkBinPath.AppendPath("bin");

          xiiDynamicArray<xiiFileStats> folders;
          xiiOSFile::GatherAllItemsInFolder(folders, windowsSdkBinPath, xiiFileSystemIteratorFlags::ReportFolders);

          folders.Sort([](const xiiFileStats& a, const xiiFileStats& b) { return a.m_sName > b.m_sName; });

          for (const xiiFileStats& folder : folders)
          {
            if (!folder.m_sName.StartsWith("10."))
            {
              continue;
            }
            rcPath = windowsSdkBinPath;
            rcPath.AppendPath(folder.m_sName);
            rcPath.AppendPath("x64/rc.exe");
            if (xiiOSFile::ExistsFile(rcPath))
            {
              break;
            }
            rcPath.Clear();
          }
        }
      }
    }
    if (!rcPath.IsEmpty())
    {
      pProjectPreferences->m_CompilerPreferences.m_sRcCompiler = rcPath;
    }
  }

  xiiString clangVersion;

  wchar_t* pProgramFiles = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &pProgramFiles)))
  {
    xiiStringBuilder clangDefaultPath;
    clangDefaultPath = xiiStringWChar(pProgramFiles);
    CoTaskMemFree(pProgramFiles);
    pProgramFiles = nullptr;

    clangDefaultPath.AppendPath("LLVM/bin/clang.exe");
    clangDefaultPath.MakeCleanPath();
    xiiStringBuilder clangCppDefaultPath = clangDefaultPath;
    clangCppDefaultPath.ReplaceLast(".exe", "++.exe");

    xiiStringView clangMajorSdkVersion = XII_PP_STRINGIFY(__clang_major__) ".";
    if (TestCompilerExecutable(clangDefaultPath, &clangVersion).Succeeded() && TestCompilerExecutable(clangCppDefaultPath).Succeeded() && clangVersion.StartsWith(clangMajorSdkVersion))
    {
      xiiStringBuilder clangNiceName;
      clangNiceName.SetFormat("Clang (system default = {})", clangVersion);
      s_MachineSpecificCompilers.PushBack({clangNiceName, xiiCompiler::Clang, clangDefaultPath, clangCppDefaultPath, false});
    }
  }
#  endif
#endif


#if XII_ENABLED(XII_PLATFORM_LINUX)
  AddCompilerVersions(s_MachineSpecificCompilers, xiiCppProject::GetSdkCompiler(), xiiCppProject::GetSdkCompilerMajorVersion());
#endif

#if XII_ENABLED(XII_COMPILER_CLANG)
  s_MachineSpecificCompilers.PushBack({"Clang (Custom)", xiiCompiler::Clang, "", "", true});
#endif

#if XII_ENABLED(XII_PLATFORM_LINUX) && XII_ENABLED(XII_COMPILER_GCC)
  s_MachineSpecificCompilers.PushBack({"Gcc (Custom)", xiiCompiler::Gcc, "", "", true});
#endif

  if (pProjectPreferences->m_CompilerPreferences.m_Compiler != sdkCompiler)
  {
    xiiStringBuilder incompatibleCompilerName = reinterpret_cast<const char*>(u8"⚠ ");
    incompatibleCompilerName.SetFormat(reinterpret_cast<const char*>(u8"⚠ {} (incompatible)"), xiiCppProject::CompilerToString(pProjectPreferences->m_CompilerPreferences.m_Compiler));
    s_MachineSpecificCompilers.PushBack({incompatibleCompilerName, pProjectPreferences->m_CompilerPreferences.m_Compiler, pProjectPreferences->m_CompilerPreferences.m_sCCompiler, pProjectPreferences->m_CompilerPreferences.m_sCppCompiler, pProjectPreferences->m_CompilerPreferences.m_bCustomCompiler});
  }
}

xiiResult xiiCppProject::ForceSdkCompatibleCompiler()
{
  xiiCppProject* pProjectPreferences = xiiPreferences::QueryPreferences<xiiCppProject>();

  xiiCompiler::Enum sdkCompiler = GetSdkCompiler();
  for (auto& compiler : s_MachineSpecificCompilers)
  {
    if (!compiler.m_bIsCustom && compiler.m_Compiler == sdkCompiler)
    {
      pProjectPreferences->m_CompilerPreferences.m_Compiler        = sdkCompiler;
      pProjectPreferences->m_CompilerPreferences.m_sCCompiler      = compiler.m_sCCompiler;
      pProjectPreferences->m_CompilerPreferences.m_sCppCompiler    = compiler.m_sCppCompiler;
      pProjectPreferences->m_CompilerPreferences.m_bCustomCompiler = compiler.m_bIsCustom;

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

xiiCppProject::~xiiCppProject() = default;
