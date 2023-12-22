#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/System/Window.h>
#include <EditorFramework/ui_WindowCfgDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>

#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtWindowCfgDlg : public QDialog, public Ui_xiiQtWindowCfgDlg
{
public:
  Q_OBJECT

public:
  xiiQtWindowCfgDlg(QWidget* pParent);

private Q_SLOTS:
  void on_m_ButtonBox_clicked(QAbstractButton* button);
  void on_m_ComboWnd_currentIndexChanged(int index);
  void on_m_CheckOverrideDefault_stateChanged(int state);

private:
  void FillUI(const xiiWindowCreationDesc& desc);
  void GrabUI(xiiWindowCreationDesc& desc);
  void UpdateUI();
  void LoadDescs();
  void SaveDescs();

  xiiUInt8              m_uiCurDesc = 0;
  xiiWindowCreationDesc m_Descs[2];
  bool                  m_bOverrideProjectDefault[2];
};
