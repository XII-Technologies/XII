#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef XII_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>

void CreateFileserveMainWindow(xiiApplication* pApp)
{
  xiiQtFileserveMainWnd* pMainWnd = new xiiQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

xiiQtFileserveMainWnd::xiiQtFileserveMainWnd(xiiApplication* pApp, QWidget* pParent) :
  QMainWindow(pParent), m_pApp(pApp)
{
  OnServerStopped();

  m_pFileserveWidget = new xiiQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  resize(700, 650);

  connect(m_pFileserveWidget, &xiiQtFileserveWidget::ServerStarted, this, &xiiQtFileserveMainWnd::OnServerStarted);
  connect(m_pFileserveWidget, &xiiQtFileserveWidget::ServerStopped, this, &xiiQtFileserveMainWnd::OnServerStopped);

  show();

  QTimer::singleShot(0, this, &xiiQtFileserveMainWnd::UpdateNetworkSlot);

  setWindowIcon(m_pFileserveWidget->windowIcon());
}


void xiiQtFileserveMainWnd::UpdateNetworkSlot()
{
  if (m_pApp->Run() == xiiApplication::Execution::Continue)
  {
    QTimer::singleShot(0, this, &xiiQtFileserveMainWnd::UpdateNetworkSlot);
  }
  else
  {
    close();
  }
}

void xiiQtFileserveMainWnd::OnServerStarted(const QString& ip, xiiUInt16 uiPort)
{
  QString title = QString("xiiFileserve (Port %1)").arg(uiPort);

  setWindowTitle(title);
}

void xiiQtFileserveMainWnd::OnServerStopped()
{
  setWindowTitle("xiiFileserve (not running)");
}

#endif
