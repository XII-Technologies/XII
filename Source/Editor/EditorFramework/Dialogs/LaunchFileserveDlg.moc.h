#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_LaunchFileserveDlg.h>
#include <Foundation/Strings/String.h>

#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtLaunchFileserveDlg : public QDialog, public Ui_xiiQtLaunchFileserveDlg
{
public:
  Q_OBJECT

public:
  xiiQtLaunchFileserveDlg(QWidget* pParent);
  ~xiiQtLaunchFileserveDlg();

  xiiString m_sFileserveCmdLine;

private Q_SLOTS:
  void on_ButtonLaunch_clicked();

private:
  virtual void showEvent(QShowEvent* event) override;
};
