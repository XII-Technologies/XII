#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <QTimer>

xiiQtWaitForOperationDlg::xiiQtWaitForOperationDlg(QWidget* parent) :
  QDialog(parent)
{
  setupUi(this);

  QTimer::singleShot(10, this, &xiiQtWaitForOperationDlg::onIdle);
}

xiiQtWaitForOperationDlg::~xiiQtWaitForOperationDlg()
{
}

void xiiQtWaitForOperationDlg::on_ButtonCancel_clicked()
{
  reject();
}

void xiiQtWaitForOperationDlg::onIdle()
{
  if (m_OnIdle())
  {
    QTimer::singleShot(10, this, &xiiQtWaitForOperationDlg::onIdle);
  }
  else
  {
    accept();
  }
}
