#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtToolBarActionMapView;
class xiiQtAssetBrowserFilter;
class xiiQtAssetBrowserModel;
struct xiiAssetCuratorEvent;

class xiiQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  xiiQtAssetBrowserWidget(QWidget* parent);
  ~xiiQtAssetBrowserWidget();

  void SetDialogMode();
  void SetSelectedAsset(xiiUuid preselectedAsset);
  void ShowOnlyTheseTypeFilters(const char* szFilters);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  xiiQtAssetBrowserModel*        GetAssetBrowserModel() { return m_pModel; }
  const xiiQtAssetBrowserModel*  GetAssetBrowserModel() const { return m_pModel; }
  xiiQtAssetBrowserFilter*       GetAssetBrowserFilter() { return m_pFilter; }
  const xiiQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemCleared();

private Q_SLOTS:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(xiiInt32 iIconSizePercentage);
  void OnSearchWidgetTextChanged(const QString& text);
  void on_ListTypeFilter_itemChanged(QListWidgetItem* item);
  void on_TreeFolderFilter_itemSelectionChanged();
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(xiiUuid preselectedAsset);
  void OnTreeOpenExplorer();
  void OnShowSubFolderItemsToggled();
  void OnShowHiddenFolderItemsToggled();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnFilterToThisPath();
  void OnListFindAllReferences(bool transitive);
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void OnModelReset();
  void OnNewAsset();

private:
  void AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  void UpdateDirectoryTree();
  void ClearDirectoryTree();
  void BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem, bool bIsHidden);
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateAssetTypes();
  void ProjectEventHandler(const xiiToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);

  bool      m_bDialogMode;
  xiiUInt32 m_uiKnownAssetFolderCount;
  bool      m_bTreeSelectionChangeInProgress = false;

  xiiQtToolBarActionMapView* m_pToolbar;
  xiiString                  m_sAllTypesFilter;
  xiiQtAssetBrowserModel*    m_pModel;
  xiiQtAssetBrowserFilter*   m_pFilter;
};
