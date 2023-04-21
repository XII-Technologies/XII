#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef XII_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>
#  include <qstylefactory.h>

void CreateFileserveMainWindow(xiiApplication* pApp)
{
  xiiQtFileserveMainWnd* pMainWnd = new xiiQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

void SetApplicationDarkTheme()
{
  QApplication::setStyle(QStyleFactory::create("fusion"));
  QPalette palette;

  palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Button, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::Light, QColor(60, 60, 60, 255));
  palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
  palette.setColor(QPalette::Dark, QColor(45, 45, 45, 255));
  palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
  palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Base, QColor(20, 20, 20, 255));
  palette.setColor(QPalette::AlternateBase, QColor(20, 20, 20, 255));
  palette.setColor(QPalette::Window, QColor(30, 30, 30, 255));
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
  palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
  QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::PlaceholderText, QColor(200, 200, 200, 255).darker());

  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(40, 40, 40, 255));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

  QApplication::setPalette(palette);
}

xiiQtFileserveMainWnd::xiiQtFileserveMainWnd(xiiApplication* pApp, QWidget* parent) :
  QMainWindow(parent), m_pApp(pApp)
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
