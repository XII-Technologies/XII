/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_TagsPage.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

#include <QWidget>

class XII_EDITORFRAMEWORK_DLL xiiQtTagsPage : public QWidget, public Ui_xiiQtTagsPage
{
public:
  Q_OBJECT

public:
  xiiQtTagsPage(QWidget* pParent);

  xiiResult ApplyChanges();

private Q_SLOTS:
  void on_ButtonNewCategory_clicked();
  void on_ButtonNewTag_clicked();
  void on_ButtonRemove_clicked();
  void on_TreeTags_itemSelectionChanged();

private:
  void      LoadTags();
  xiiResult SaveTags();
  void      FillList();
  void      GetTagsFromList();

  QTreeWidgetItem* CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag, bool bBuiltIn);

  xiiHybridArray<xiiToolsTag, 32>     m_Tags;
  xiiMap<xiiString, QTreeWidgetItem*> m_CategoryToItem;
};
