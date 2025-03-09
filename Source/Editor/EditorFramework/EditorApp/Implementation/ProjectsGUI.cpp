#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void xiiQtEditorApp::GuiOpenDashboard()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDashboard", Qt::ConnectionType::QueuedConnection);
}

void xiiQtEditorApp::GuiOpenDocsAndCommunity()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDocsAndCommunity", Qt::ConnectionType::QueuedConnection);
}

bool xiiQtEditorApp::GuiCreateProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(true);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
    return true;
  }
}

bool xiiQtEditorApp::GuiOpenProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(false);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
    return true;
  }
}

void xiiQtEditorApp::SlotQueuedGuiOpenDashboard()
{
  xiiQtDashboardDlg dlg(nullptr, xiiQtDashboardDlg::DashboardTab::Projects);
  dlg.exec();
}

void xiiQtEditorApp::SlotQueuedGuiOpenDocsAndCommunity()
{
  xiiQtDashboardDlg dlg(nullptr, xiiQtDashboardDlg::DashboardTab::Documentation);
  dlg.exec();
}

void xiiQtEditorApp::SlotQueuedGuiCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

bool xiiQtEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString    sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  xiiStringBuilder sFile;

  const char* szFilter = "xiiProject (xiiProject)";

  if (bCreate)
  {
    xiiQtCreateProjectDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Rejected)
      return false;

    sFile = dlg.GetFullTargetPath();
  }
  else
  {
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter), nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();
  }

  if (sFile.IsEmpty())
    return false;

  if (bCreate)
  {
    sFile.AppendPath("xiiProject");
  }

  m_sLastProjectFolder = xiiPathUtils::GetFileDirectory(sFile);

  return CreateOrOpenProject(bCreate, sFile).Succeeded();
}
