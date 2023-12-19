#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

xiiQtLaunchFileserveDlg::xiiQtLaunchFileserveDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);
}

xiiQtLaunchFileserveDlg::~xiiQtLaunchFileserveDlg() = default;

void xiiQtLaunchFileserveDlg::showEvent(QShowEvent* event)
{
  xiiStringBuilder sCmdLine = xiiQtEditorApp::GetSingleton()->BuildFileserveCommandLine();
  EditFileserve->setPlainText(sCmdLine.GetData());

  QDialog::showEvent(event);
}

void xiiQtLaunchFileserveDlg::on_ButtonLaunch_clicked()
{
  xiiQtEditorApp::GetSingleton()->RunFileserve();
  accept();
}
