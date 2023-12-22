#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_TagsDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtTagsDlg : public QDialog, public Ui_xiiQtTagsDlg
{
public:
  Q_OBJECT

public:
  xiiQtTagsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonNewCategory_clicked();
  void on_ButtonNewTag_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeTags_itemSelectionChanged();

private:
  void LoadTags();
  void SaveTags();
  void FillList();
  void GetTagsFromList();

  QTreeWidgetItem* CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag, bool bBuiltIn);

  xiiHybridArray<xiiToolsTag, 32>     m_Tags;
  xiiMap<xiiString, QTreeWidgetItem*> m_CategoryToItem;
};
