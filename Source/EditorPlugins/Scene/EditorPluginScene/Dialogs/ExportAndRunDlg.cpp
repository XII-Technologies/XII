#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool xiiQtExportAndRunDlg::s_bTransformAll    = true;
bool xiiQtExportAndRunDlg::s_bUpdateThumbnail = false;
bool xiiQtExportAndRunDlg::s_bCompileCpp      = true;

static int s_iLastPlayerApp = 0;

xiiQtExportAndRunDlg::xiiQtExportAndRunDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  ToolCombo->addItem("xiiPlayer", "Player.exe");
#else
  ToolCombo->addItem("xiiPlayer", "Player");
#endif

  xiiProjectPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiProjectPreferencesUser>();

  for (const auto& app : pPref->m_PlayerApps)
  {
    xiiStringBuilder name = xiiPathUtils::GetFileName(app);

    ToolCombo->addItem(name.GetData(), QString::fromUtf8(app.GetData()));
  }

  ToolCombo->setCurrentIndex(s_iLastPlayerApp);

  m_CppSettings.Load().IgnoreResult();
}

void xiiQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll    = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
  s_iLastPlayerApp   = ToolCombo->currentIndex();
  s_bCompileCpp      = CompileCpp->isChecked();

  xiiProjectPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiProjectPreferencesUser>();
  pPref->m_PlayerApps.Clear();

  for (int i = 1; i < ToolCombo->count(); ++i)
  {
    xiiStringBuilder path = ToolCombo->itemData(i).toString().toUtf8().data();
    path.MakeCleanPath();

    pPref->m_PlayerApps.PushBack(path);
  }
}

void xiiQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdateThumbnail->setVisible(m_bShowThumbnailCheckbox);
  TransformAll->setChecked(s_bTransformAll);
  UpdateThumbnail->setChecked(s_bUpdateThumbnail);
  PlayerCmdLine->setPlainText(m_sCmdLine.GetData());

  if (!xiiCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(s_bCompileCpp);
  }
}

void xiiQtExportAndRunDlg::on_ExportOnly_clicked()
{
  PullFromUI();
  m_bRunAfterExport = false;
  accept();
}

void xiiQtExportAndRunDlg::on_ExportAndRun_clicked()
{
  PullFromUI();
  m_bRunAfterExport = true;
  m_sApplication    = ToolCombo->currentData().toString().toUtf8().data();
  accept();
}

void xiiQtExportAndRunDlg::on_AddToolButton_clicked()
{
  xiiStringBuilder appDir = xiiOSFile::GetApplicationDirectory();
  appDir.MakeCleanPath();
  static QString sLastPath = appDir.GetData();

  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Applicaation (*.exe)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  sLastPath = sFile;

  xiiStringBuilder path = sFile.toUtf8().data();
  path.MakeCleanPath();
  path.TrimWordStart(appDir);
  path.Trim("/", "");

  xiiStringBuilder tmp;
  ToolCombo->addItem(QString::fromUtf8(path.GetFileName().GetData(tmp)), QString::fromUtf8(path.GetData()));
  ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
}

void xiiQtExportAndRunDlg::on_RemoveToolButton_clicked()
{
  ToolCombo->removeItem(ToolCombo->currentIndex());
}

void xiiQtExportAndRunDlg::on_ToolCombo_currentIndexChanged(int idx)
{
  RemoveToolButton->setEnabled(idx != 0);
}
