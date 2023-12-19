#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_InputConfigDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <QDialog>

class QTreeWidgetItem;

class XII_EDITORFRAMEWORK_DLL xiiQtInputConfigDlg : public QDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  xiiQtInputConfigDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewInputSet_clicked();
  void on_ButtonNewAction_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeActions_itemSelectionChanged();

private:
  void LoadActions();
  void SaveActions();
  void FillList();
  void GetActionsFromList();

  QTreeWidgetItem* CreateActionItem(QTreeWidgetItem* pParentItem, const xiiGameAppInputConfig& action);

  xiiMap<xiiString, QTreeWidgetItem*>       m_InputSetToItem;
  xiiHybridArray<xiiGameAppInputConfig, 32> m_Actions;
  xiiDynamicArray<xiiString>                m_AllInputSlots;
};
