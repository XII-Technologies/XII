#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

#include <QFileDialog>

bool xiiQtExportProjectDlg::s_bTransformAll = true;

xiiQtExportProjectDlg::xiiQtExportProjectDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  xiiProjectPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void xiiQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);

  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(true);
  }
}

void xiiQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
    xiiProjectPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiProjectPreferencesUser>();
    pPref->m_sExportFolder           = sPath.toUtf8().data();
  }
}

void xiiQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // filter out unused runtime/game plugins
  // select asset profile for export
  // copy inputs into resource: RML files

  if (CompileCpp->isChecked())
  {
    if (xiiCppProject::EnsureCppPluginReady().Failed())
      return;
  }

  if (TransformAll->isChecked())
  {
    xiiStatus stat = xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::TriggeredManually);

    if (stat.Failed())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  const xiiString szDstFolder = Destination->text().toUtf8().data();

  xiiLogSystemToBuffer logFile;

  auto WriteLogFile = [&]() {
    xiiStringBuilder sTemp;
    sTemp.Set(szDstFolder, "/ExportLog.txt");

    ExportLog->setPlainText(logFile.m_sBuffer.GetData());

    xiiOSFile file;

    if (file.Open(sTemp, xiiFileOpenMode::Write).Failed())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxWarning(xiiFmt("Failed to write export log '{0}'", sTemp));
      return;
    }

    file.Write(logFile.m_sBuffer.GetData(), logFile.m_sBuffer.GetElementCount()).AssertSuccess();
  };

  xiiLogSystemScope logScope(&logFile);
  XII_SCOPE_EXIT(WriteLogFile());

  if (xiiProjectExport::ExportProject(szDstFolder, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), xiiQtEditorApp::GetSingleton()->GetFileSystemConfig()).Failed())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxWarning("Project export failed. See log for details.");
  }
  else
  {
    xiiQtUiServices::GetSingleton()->MessageBoxInformation("Project export successful.");
    xiiQtUiServices::GetSingleton()->OpenInExplorer(szDstFolder, false);
  }
}
