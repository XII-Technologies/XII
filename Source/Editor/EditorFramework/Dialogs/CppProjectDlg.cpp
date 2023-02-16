#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiQtCppProjectDlg::xiiQtCppProjectDlg(QWidget* parent) :
  QDialog(parent)
{
  setupUi(this);

  Generator->addItem("Visual Studio 2019");
  Generator->addItem("Visual Studio 2022");
  Generator->setCurrentIndex(1);

  UpdateUI();
}

xiiResult xiiQtCppProjectDlg::GenerateSolution()
{
  if (xiiSystemInformation::IsDebuggerAttached())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake can fail with the error that no C/C++ compiler can be found.");
  }

  xiiProgressRange progress("Generating Solution", 4, false);
  progress.SetStepWeighting(0, 0.05f);
  progress.SetStepWeighting(1, 0.1f);
  progress.SetStepWeighting(2, 0.1f);
  progress.SetStepWeighting(3, 1.0f);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  OutputLog->clear();
  xiiStringBuilder output;
  XII_SCOPE_EXIT(OutputLog->setText(output.GetData()));
  XII_SCOPE_EXIT(QApplication::restoreOverrideCursor());
  XII_SCOPE_EXIT(UpdateUI());

  const xiiString sRealProjectName = xiiToolsProject::GetSingleton()->GetProjectName(false);
  const xiiString sProjectName     = xiiToolsProject::GetSingleton()->GetProjectName(true);

  if (sRealProjectName != sProjectName)
  {
    if (xiiQtUiServices::MessageBoxQuestion(xiiFmt("The project's name '{}' contains characters and/or whitespace that can't be used in C++ identifiers.\n\nInstead the adjusted name '{}' will be used.\n\nIf you don't want this, you need to rename your project (rename the folder in which it resides) and try again.\n\nDo you want to continue?", sRealProjectName, sProjectName), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
    {
      output.AppendFormat("User doesn't like auto-generated project name '{}' :(", sProjectName);
      return XII_FAILURE;
    }
  }

  xiiStringBuilder sProjectNameUpper = sProjectName;
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
    progress.BeginNextStep("Gathering source files");

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
    progress.BeginNextStep("Copying sources");

    for (const auto& ftc : filesCopied)
    {
      if (xiiOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        output.AppendFormat("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return XII_FAILURE;
      }
    }
  }

  // Modify sources
  {
    progress.BeginNextStep("Modifying sources");

    for (const auto& filePath : filesCopied)
    {
      xiiStringBuilder content;

      {
        xiiFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          output.AppendFormat("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return XII_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("SourceTemplate", sProjectName);
      content.ReplaceAll("SOURCE_TEMPLATE", sProjectNameUpper);

      {
        xiiFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          output.AppendFormat("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return XII_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  // Invoke CMake
  {
    progress.BeginNextStep("Invoking CMake");

    const xiiString sSdkDir       = xiiFileSystem::GetSdkRootDirectory();
    const xiiString sBuildDir     = GetBuildDir();
    const xiiString sSolutionFile = GetSolutionFile();

    if (xiiOSFile::ExistsDirectory(sBuildDir) && xiiOSFile::DeleteFolder(sBuildDir).Failed())
    {
      output.AppendFormat("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", sBuildDir);
    }

    xiiStringBuilder tmp;

    QStringList args;
    args << "-S";
    args << GetTargetDir().GetData();

    tmp.Format("-DXII_SDK_DIR:PATH={}", sSdkDir);
    args << tmp.GetData();

    tmp.Format("-DXII_BUILDTYPE_ONLY:STRING={}", BUILDSYSTEM_BUILDTYPE);
    args << tmp.GetData();

    args << "-G";
    args << GetGeneratorCMake().GetData();

    args << "-B";
    args << sBuildDir.GetData();

    args << "-A";
    args << "x64";

    xiiLogSystemToBuffer log;

    xiiStatus res = xiiQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake", args, 120, &log, xiiLogMsgType::InfoMsg);

    if (res.Failed())
    {
      output.AppendFormat("Solution generation failed:\n\n");
      output.AppendFormat("{}\n", log.m_sBuffer);
      output.AppendFormat("{}\n", res.m_sMessage);
      return XII_FAILURE;
    }

    output.AppendFormat("Generated solution successfully.\n\n");
    output.AppendFormat("{}\n", log.m_sBuffer);
  }

  return XII_SUCCESS;
}

void xiiQtCppProjectDlg::on_Result_rejected()
{
  reject();
}

void xiiQtCppProjectDlg::on_OpenPluginLocation_clicked()
{
  xiiQtUiServices::OpenInExplorer(PluginLocation->text().toUtf8().data(), false);
}

void xiiQtCppProjectDlg::on_OpenBuildFolder_clicked()
{
  xiiQtUiServices::OpenInExplorer(BuildFolder->text().toUtf8().data(), false);
}

void xiiQtCppProjectDlg::on_Generator_currentIndexChanged(int)
{
  UpdateUI();
}

void xiiQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (!xiiQtUiServices::OpenFileInDefaultProgram(GetSolutionFile()))
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
  }
}

void xiiQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (xiiOSFile::ExistsFile(GetSolutionFile()))
  {
    if (xiiQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
      return;
  }

  if (GenerateSolution().Failed())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed. Check the log output for details.");
  }
  else
  {
    xiiStringBuilder txt;
    txt.Format("The solution was generated successfully.\n\nDo you want to open the solution now?");

    if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion(txt, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
      on_OpenSolution_clicked();
    }

    xiiStringBuilder sProjectName = xiiToolsProject::GetSingleton()->GetProjectName(true);
    xiiStringBuilder sPluginName(sProjectName, "Plugin");

    xiiPluginBundleSet& bundles = xiiQtEditorApp::GetSingleton()->GetPluginBundles();

    bundles.m_Plugins.Remove(sPluginName);
    xiiPluginBundle& plugin       = bundles.m_Plugins[sPluginName];
    plugin.m_bLoadCopy            = true;
    plugin.m_bSelected            = true;
    plugin.m_bMissing             = true;
    plugin.m_LastModificationTime = xiiTimestamp::CurrentTimestamp();
    plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
    txt.Set("'", sProjectName, "' project plugin");
    plugin.m_sDisplayName = txt;
    txt.Set("C++ code for the '", sProjectName, "' project.");
    plugin.m_sDescription = txt;
    plugin.m_RuntimePlugins.PushBack(sPluginName);

    xiiQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
  }
}

void xiiQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(GetTargetDir().GetData());
  BuildFolder->setText(GetBuildDir().GetData());

  OpenPluginLocation->setEnabled(xiiOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(xiiOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(xiiOSFile::ExistsFile(GetSolutionFile()));
}

xiiString xiiQtCppProjectDlg::GetTargetDir() const
{
  xiiStringBuilder sTargetDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  sTargetDir.AppendPath("Source");

  return sTargetDir;
}

xiiString xiiQtCppProjectDlg::GetBuildDir() const
{
  xiiStringBuilder sBuildDir;
  sBuildDir.Format("{}/Build/{}", xiiToolsProject::GetSingleton()->GetProjectDirectory(), GetGeneratorFolder());

  return sBuildDir;
}

xiiString xiiQtCppProjectDlg::GetSolutionFile() const
{
  xiiStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir();
  sSolutionFile.AppendPath(xiiToolsProject::GetSingleton()->GetProjectName(true));
  sSolutionFile.Append(".sln");

  return sSolutionFile;
}

xiiString xiiQtCppProjectDlg::GetGeneratorCMake() const
{
  switch (Generator->currentIndex())
  {
    case 0:
      return "Visual Studio 16 2019";
    case 1:
      return "Visual Studio 17 2022";

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

xiiString xiiQtCppProjectDlg::GetGeneratorFolder() const
{
  switch (Generator->currentIndex())
  {
    case 0:
      return "Vs2019x64";
    case 1:
      return "Vs2022x64";

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}
