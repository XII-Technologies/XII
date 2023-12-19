#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/ui_CppProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtCppProjectDlg : public QDialog, public Ui_xiiQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  xiiQtCppProjectDlg(QWidget* pParent);

private Q_SLOTS:
  void on_Result_rejected();
  void on_OpenPluginLocation_clicked();
  void on_OpenBuildFolder_clicked();
  void on_Generator_currentIndexChanged(int);
  void on_OpenSolution_clicked();
  void on_GenerateSolution_clicked();
  void on_PluginName_textEdited(const QString& text);

private:
  void UpdateUI();

  xiiCppSettings m_OldCppSettings;
  xiiCppSettings m_CppSettings;
};
