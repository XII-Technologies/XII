/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_InputConfigPage.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <GameEngine/Configuration/InputConfig.h>

#include <QWidget>

class QTreeWidgetItem;

class XII_EDITORFRAMEWORK_DLL xiiQtInputConfigPage : public QWidget, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  xiiQtInputConfigPage(QWidget* pParent);

  xiiResult ApplyChanges();

Q_SIGNALS:
  void Changed();

private Q_SLOTS:
  void on_ButtonNewInputSet_clicked();
  void on_ButtonNewAction_clicked();
  void on_ButtonRemove_clicked();
  void on_TreeActions_itemSelectionChanged();

private:
  void      LoadActions();
  xiiResult SaveActions();
  void      FillList();
  void      GetActionsFromList();

  QTreeWidgetItem* CreateActionItem(QTreeWidgetItem* pParentItem, const xiiGameAppInputConfig& action);

  xiiMap<xiiString, QTreeWidgetItem*>       m_InputSetToItem;
  xiiHybridArray<xiiGameAppInputConfig, 32> m_Actions;
  xiiDynamicArray<xiiString>                m_AllInputSlots;
};
