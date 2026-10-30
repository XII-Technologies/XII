/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_DataDirsPage.h>
#include <Foundation/Application/Config/FileSystemConfig.h>

#include <QWidget>

class XII_EDITORFRAMEWORK_DLL xiiQtDataDirsPage : public QWidget, public Ui_xiiQtDataDirsPage
{
public:
  Q_OBJECT

public:
  xiiQtDataDirsPage(QWidget* pParent);

  xiiResult ApplyChanges();

private Q_SLOTS:
  void on_ButtonUp_clicked();
  void on_ButtonDown_clicked();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOpenFolder_clicked();
  void on_ListDataDirs_itemSelectionChanged();
  void on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem);

private:
  void FillList();

  xiiInt32                       m_iSelection;
  xiiApplicationFileSystemConfig m_Config;
};
