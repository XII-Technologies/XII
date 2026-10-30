/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/System/Window.h>
#include <EditorFramework/ui_WindowConfigPage.h>
#include <Foundation/Application/Config/FileSystemConfig.h>

#include <QByteArray>
#include <QWidget>

class XII_EDITORFRAMEWORK_DLL xiiQtWindowConfigPage : public QWidget, public Ui_xiiQtWindowConfigPage
{
public:
  Q_OBJECT

public:
  xiiQtWindowConfigPage(QWidget* pParent);

  xiiResult ApplyChanges();

private Q_SLOTS:
  void on_m_ComboWnd_currentIndexChanged(int index);
  void on_m_CheckOverrideDefault_stateChanged(int state);

private:
  void      FillUI(const xiiWindowCreationDescription& desc);
  void      GrabUI(xiiWindowCreationDescription& desc);
  void      UpdateUI();
  void      LoadDescs();
  xiiResult SaveDescs();

  xiiUInt8                     m_uiCurDesc = 0;
  xiiWindowCreationDescription m_Descs[2];
  bool                         m_bOverrideProjectDefault[2];
  QByteArray                   m_Committed[2];
  bool                         m_bCommittedOverride = false;
  bool                         m_bCanSave[2]        = {true, true};
};
