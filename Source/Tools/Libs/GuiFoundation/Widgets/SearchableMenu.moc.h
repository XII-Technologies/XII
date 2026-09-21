/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidgetAction>

class xiiQtSearchWidget;
class QTreeWidget;
class QTreeWidgetItem;
class xiiQtTreeSearchFilterModel;
class QStandardItemModel;
class QTreeView;
class QStandardItem;

/// Implements an item for insertion into a QMenu that shows a search bar and a hierarchical list of options.
///
/// Fill the searchable menu object with items (use slashes to indicate hierarchy) then use QMenu::addAction to insert it
/// into another QMenu.
/// Connect to MenuItemTriggered() to handle the item activation and also call QMenu::close() on the parent menu.
class XII_GUIFOUNDATION_DLL xiiQtSearchableMenu : public QWidgetAction
{
  Q_OBJECT
public:
  /// The parent should usually be a QMenu into which this QWidgetAction is inserted as an action.
  xiiQtSearchableMenu(QObject* pParent);

  /// Use slashes in the sInternalPath to separate sub-items.
  void AddItem(xiiStringView sDisplayName, xiiStringView sInternalPath, const QVariant& variant, QIcon icon = QIcon());

  /// Returns the currently entered search text.
  QString GetSearchText() const;

  /// Sets up the internal data model and ensures the menu's search bar gets input focus. Do this after adding the item to the parent menu.
  void Finalize(const QString& sSearchText);

Q_SIGNALS:
  /// Signaled when an item is double clicked or otherwise selected for activation.
  void MenuItemTriggered(const QString& sName, const QVariant& variant);

  /// Triggered whenever the search text is modified.
  void SearchTextChanged(const QString& sText);

private Q_SLOTS:
  void OnItemActivated(const QModelIndex& index);
  void OnEnterPressed();
  void OnSpecialKeyPressed(Qt::Key key);
  void OnSearchChanged(const QString& text);
  void OnShow();

protected:
  virtual bool eventFilter(QObject*, QEvent*) override;

private:
  QStandardItem* CreateCategoryMenu(xiiStringView sCategory);
  bool           SelectFirstLeaf(QModelIndex parent);

  QWidget*                          m_pGroup;
  xiiQtSearchWidget*                m_pSearch;
  xiiQtTreeSearchFilterModel*       m_pFilterModel;
  QTreeView*                        m_pTreeView;
  QStandardItemModel*               m_pItemModel;
  xiiMap<xiiString, QStandardItem*> m_Hierarchy;
};
