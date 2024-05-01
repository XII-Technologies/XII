#include <EditorFramework/EditorFrameworkPCH.h>

#include "Foundation/Logging/Log.h"
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiQtCppProjectDlg::xiiQtCppProjectDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  m_OldCppSettings.Load().IgnoreResult();
  m_CppSettings.Load().IgnoreResult();

  {
    xiiQtScopedBlockSignals _1(PluginName);
    PluginName->setPlaceholderText(xiiToolsProject::GetSingleton()->GetProjectName(true).GetData());
    PluginName->setText(m_CppSettings.m_sPluginName.GetData());
  }

  {
    xiiQtScopedBlockSignals _1(Generator);
    Generator->addItem("None");
    Generator->addItem("Visual Studio 2022");
    Generator->setCurrentIndex(0);

    if (m_CppSettings.m_Compiler == xiiCppSettings::Compiler::Vs2022)
    {
      Generator->setCurrentIndex(1);
    }
  }

  UpdateUI();
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
  switch (Generator->currentIndex())
  {
    case 0:
      m_CppSettings.m_Compiler = xiiCppSettings::Compiler::None;
      break;
    case 1:
      m_CppSettings.m_Compiler = xiiCppSettings::Compiler::Vs2022;
      break;
  }

  UpdateUI();
}

void xiiQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (!xiiQtUiServices::OpenFileInDefaultProgram(xiiCppProject::GetSolutionPath(m_CppSettings)))
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
  }
}

void xiiQtCppProjectDlg::on_PluginName_textEdited(const QString& text)
{
  xiiStringBuilder name = PluginName->text().toUtf8().data();

  if (name.EndsWith_NoCase("Plugin"))
  {
    name.Shrink(0, 6);
  }

  m_CppSettings.m_sPluginName = name;

  UpdateUI();
}

void xiiQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(xiiCppProject::GetTargetSourceDir().GetData());
  BuildFolder->setText(xiiCppProject::GetBuildDir(m_CppSettings).GetData());

  GenerateSolution->setEnabled(m_CppSettings.m_Compiler != xiiCppSettings::Compiler::None);
  OpenPluginLocation->setEnabled(xiiOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(xiiOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(xiiCppProject::ExistsSolution(m_CppSettings));
}

class xiiForwardToQTextEdit : public xiiLogInterface
{
public:
  QTextEdit* m_pTextEdit = nullptr;

  void HandleLogMessage(const xiiLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case xiiLogMsgType::GlobalDefault:
      case xiiLogMsgType::Flush:
      case xiiLogMsgType::BeginGroup:
      case xiiLogMsgType::EndGroup:
      case xiiLogMsgType::None:
      case xiiLogMsgType::All:
      case xiiLogMsgType::ENUM_COUNT:
        return;

      case xiiLogMsgType::ErrorMsg:
      case xiiLogMsgType::SeriousWarningMsg:
      case xiiLogMsgType::WarningMsg:
      case xiiLogMsgType::SuccessMsg:
      case xiiLogMsgType::InfoMsg:
      case xiiLogMsgType::DevMsg:
      case xiiLogMsgType::DebugMsg:
      {
        xiiStringBuilder tmp(le.m_sText, "\n");

        QString s = m_pTextEdit->toPlainText();
        s.append(tmp);
        m_pTextEdit->setText(s);
        return;
      }

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
};

void xiiQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (xiiCppProject::ExistsSolution(m_CppSettings))
  {
    if (xiiQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.m_sPluginName.IsEmpty())
  {
    m_CppSettings.m_sPluginName = PluginName->placeholderText().toUtf8().data();
  }

  if (!m_OldCppSettings.m_sPluginName.IsEmpty() && m_OldCppSettings.m_sPluginName != m_CppSettings.m_sPluginName)
  {
    if (xiiQtUiServices::MessageBoxQuestion("You are attempting to change the name of the existing C++ plugin.\n\nTHIS IS A BAD IDEA.\n\nThe C++ sources and CMake files were already created with the old name in it. To not accidentally delete your work, XII won't touch any of those files. Therefore this change won't have any effect, unless you have already deleted those files yourself and XII can just create new ones. Only select YES if you have done the necessary steps and/or know what you are doing.", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.Save().Failed())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("Saving new C++ project settings failed.");
    return;
  }

  m_OldCppSettings.Load().IgnoreResult();

  if (xiiSystemInformation::IsDebuggerAttached())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake usually fails with the error that no C/C++ compiler can be found.\n\nDetach the debugger now, then press OK to continue.");
  }

  OutputLog->clear();

  {
    xiiForwardToQTextEdit log;
    log.m_pTextEdit = OutputLog;
    xiiLogSystemScope _logScope(&log);

    xiiProgressRange progress("Generating Solution", 3, false);
    progress.SetStepWeighting(0, 0.1f);
    progress.SetStepWeighting(1, 0.1f);
    progress.SetStepWeighting(2, 0.8f);

    XII_SCOPE_EXIT(UpdateUI());

    {
      progress.BeginNextStep("Clean Build Directory");

      if (xiiCppProject::CleanBuildDir(m_CppSettings).Failed())
      {
        xiiLog::Warning("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", xiiCppProject::GetBuildDir(m_CppSettings));
      }
    }

    {
      progress.BeginNextStep("Populate with Default Sources");
      if (xiiCppProject::PopulateWithDefaultSources(m_CppSettings).Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxWarning("Failed to populate the Source directory with the default files.\n\nCheck the log for details.");
        return;
      }
    }

    // run CMake
    {
      progress.BeginNextStep("Running CMake");

      if (xiiCppProject::RunCMake(m_CppSettings).Failed())
      {

        xiiQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed.\n\nCheck the log for details.");
        return;
      }
    }

    if (xiiCppProject::BuildCodeIfNecessary(m_CppSettings).Failed())
    {
      xiiLog::Error("Failed to compile the newly generated C++ solution.");
    }
  }

  xiiCppProject::UpdatePluginConfig(m_CppSettings);

  xiiQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion("The solution was generated successfully.\n\nDo you want to open it now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
  {
    on_OpenSolution_clicked();
  }
}
