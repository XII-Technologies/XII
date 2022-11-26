#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_PluginSelectionDlg.h>
#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtPluginSelectionDlg : public QDialog, public Ui_PluginSelectionDlg
{
public:
  Q_OBJECT

public:
  xiiQtPluginSelectionDlg(xiiPluginBundleSet* pPluginSet, QWidget* parent = nullptr);
  ~xiiQtPluginSelectionDlg();


private Q_SLOTS:
  void on_Buttons_clicked(QAbstractButton* pButton);

private:
  xiiPluginBundleSet  m_LocalPluginSet;
  xiiPluginBundleSet* m_pPluginSet = nullptr;
};
