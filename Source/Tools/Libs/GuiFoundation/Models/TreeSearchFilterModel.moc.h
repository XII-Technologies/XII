#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Containers/Map.h>
#include <QSortFilterProxyModel>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

class QWidget;

class XII_GUIFOUNDATION_DLL xiiQtTreeSearchFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  xiiQtTreeSearchFilterModel(QWidget* pParent);

  void SetFilterText(const QString& sText);

  /// \brief By default only nodes (and their parents) are shown that fit the search criterion.
  /// If this is enabled, all child nodes of nodes that fit the criterion are included as well.
  void SetIncludeChildren(bool bInclude);

protected:
  void         RecomputeVisibleItems();
  bool         UpdateVisibility(const QModelIndex& idx, bool bParentIsVisible);
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

  bool                      m_bIncludeChildren;
  QAbstractItemModel*       m_pSourceModel;
  xiiSearchPatternFilter    m_Filter;
  xiiMap<QModelIndex, bool> m_Visible;
};
