#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/SourceGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

xiiEvent<const xiiCppSettings&> xiiCppProject::s_ChangeEvents;

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
  switch (cfg.m_Compiler)
  {
    case xiiCppSettings::Compiler::None:
      return "";

    case xiiCppSettings::Compiler::Vs2019:
      return "Vs2019x64";

    case xiiCppSettings::Compiler::Vs2022:
      return "Vs2022x64";

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

xiiString xiiCppProject::GetCMakeGeneratorName(const xiiCppSettings& cfg)
{
  switch (cfg.m_Compiler)
  {
    case xiiCppSettings::Compiler::None:
      return "";

    case xiiCppSettings::Compiler::Vs2019:
      return "Visual Studio 16 2019";

    case xiiCppSettings::Compiler::Vs2022:
      return "Visual Studio 17 2022";

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
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
  sBuildDir.Format("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

xiiString xiiCppProject::GetSolutionPath(const xiiCppSettings& cfg)
{
  xiiStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);
  sSolutionFile.AppendPath(cfg.m_sPluginName);
  sSolutionFile.Append(".sln");
  return sSolutionFile;
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

      // Do not copy files over that already exist (and may have edits)
      if (xiiOSFile::ExistsFile(dstPath))
      {
        // If any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
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

  const xiiString sSdkDir       = xiiFileSystem::GetSdkRootDirectory();
  const xiiString sBuildDir     = xiiCppProject::GetBuildDir(cfg);
  const xiiString sSolutionFile = xiiCppProject::GetSolutionPath(cfg);

  xiiStringBuilder tmp;

  QStringList args;
  args << "-S";
  args << xiiCppProject::GetTargetSourceDir().GetData();

  tmp.Format("-DXII_SDK_DIR:PATH={}", sSdkDir);
  args << tmp.GetData();

  tmp.Format("-DXII_BUILDTYPE_ONLY:STRING={}", BUILDSYSTEM_BUILDTYPE);
  args << tmp.GetData();

  args << "-G";
  args << xiiCppProject::GetCMakeGeneratorName(cfg).GetData();

  args << "-B";
  args << sBuildDir.GetData();

  args << "-A";
  args << "x64";

  xiiLogSystemToBuffer log;

  xiiStatus res = xiiQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake", args, 120, &log, xiiLogMsgType::InfoMsg);

  if (res.Failed())
  {
    xiiLog::Error("Solution generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.m_sMessage);
    return XII_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    xiiLog::Error("CMake did not generate the expected solution. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return XII_FAILURE;
  }

  xiiLog::Success("Solution generated.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return XII_SUCCESS;
}

xiiResult xiiCppProject::RunCMakeIfNecessary(const xiiCppSettings& cfg)
{
  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
    return XII_SUCCESS;

  if (xiiCppProject::ExistsSolution(cfg) && xiiCppProject::CheckCMakeCache(cfg))
    return XII_SUCCESS;

  return xiiCppProject::RunCMake(cfg);
}

xiiResult xiiCppProject::CompileSolution(const xiiCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  XII_LOG_BLOCK("Compile Solution");

  if (cfg.m_sMsBuildPath.IsEmpty())
  {
    xiiLog::Error("MSBuild path is not available.");
    return XII_FAILURE;
  }

  if (xiiSystemInformation::IsDebuggerAttached())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }

  xiiHybridArray<xiiString, 32> errors;

  xiiProcessOptions po;
  po.m_sProcess           = cfg.m_sMsBuildPath;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut           = [&](xiiStringView res) {
    if (res.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(res);
  };

  po.AddArgument(xiiCppProject::GetSolutionPath(cfg));
  po.AddArgument("/m"); // Multi-threaded compilation
  po.AddArgument("/nr:false");
  po.AddArgument("/t:Build");
  po.AddArgument("/p:Configuration={}", BUILDSYSTEM_BUILDTYPE);
  po.AddArgument("/p:Platform=x64");

  xiiStringBuilder sMsBuildCmd;
  po.BuildCommandLineString(sMsBuildCmd);
  xiiLog::Dev("Running MSBuild: {}", sMsBuildCmd);

  xiiInt32 iReturnCode = 0;
  if (xiiProcess::Execute(po, &iReturnCode).Failed())
  {
    xiiLog::Error("MSBuild failed to run.");
    return XII_FAILURE;
  }

  if (iReturnCode == 0)
  {
    xiiLog::Success("Compiled C++ solution.");
    return XII_SUCCESS;
  }

  xiiLog::Error("MSBuild failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    xiiLog::Error(err);
  }

  return XII_FAILURE;
}

xiiResult xiiCppProject::BuildCodeIfNecessary(const xiiCppSettings& cfg)
{
  XII_SUCCEED_OR_RETURN(xiiCppProject::FindMsBuild(cfg));

  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
    return XII_SUCCESS;

  if (!xiiCppProject::ExistsSolution(cfg) || !xiiCppProject::CheckCMakeCache(cfg))
  {
    XII_SUCCEED_OR_RETURN(xiiCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

xiiResult xiiCppProject::FindMsBuild(const xiiCppSettings& cfg)
{
  if (!cfg.m_sMsBuildPath.IsEmpty())
    return XII_SUCCESS;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  xiiStringBuilder sVsWhere;

  wchar_t* pPath = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, nullptr, &pPath)))
  {
    sVsWhere = xiiStringWChar(pPath);
    sVsWhere.AppendPath("Microsoft Visual Studio/Installer/vswhere.exe");

    CoTaskMemFree(pPath);
  }
  else
  {
    xiiLog::Error("Could not find the 'Program Files (x86)' folder.");
    return XII_FAILURE;
  }

  xiiStringBuilder  sStdOut;
  xiiProcessOptions po;
  po.m_sProcess           = sVsWhere;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut           = [&](xiiStringView res) {
    sStdOut.Append(res);
  };

  // TODO: search for VS2022 or VS2019 depending on cfg
  po.AddCommandLine("-latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");

  if (xiiProcess::Execute(po).Failed())
  {
    xiiLog::Error("Executing vsWhere.exe failed. Do you have the correct version of Visual Studio installed?");
    return XII_FAILURE;
  }

  sStdOut.Trim("\n\r");
  sStdOut.MakeCleanPath();

  cfg.m_sMsBuildPath = sStdOut;
  return XII_SUCCESS;
#else
  return XII_FAILURE;
#endif
}

void xiiCppProject::UpdatePluginConfig(const xiiCppSettings& cfg)
{
  const xiiStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  xiiPluginBundleSet& bundles = xiiQtEditorApp::GetSingleton()->GetPluginBundles();

  xiiStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  xiiPluginBundle& plugin = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy      = true;
  plugin.m_bSelected      = true;
  plugin.m_bMissing       = true;
  plugin.m_LastModificationTime.Invalidate();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  xiiQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}
