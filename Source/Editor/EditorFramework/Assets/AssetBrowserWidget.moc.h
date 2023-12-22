#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtToolBarActionMapView;
class xiiQtAssetBrowserFilter;
class xiiQtAssetBrowserModel;
struct xiiAssetCuratorEvent;
class xiiQtAssetBrowserModel;

class xiiQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  xiiQtAssetBrowserWidget(QWidget* pParent);
  ~xiiQtAssetBrowserWidget();

  enum class Mode
  {
    Browser,
    AssetPicker,
    FilePicker,
  };

  void SetMode(Mode mode);
  void SetSelectedAsset(xiiUuid preselectedAsset);
  void SetSelectedFile(xiiStringView sAbsPath);
  void ShowOnlyTheseTypeFilters(xiiStringView sFilters);
  void UseFileExtensionFilters(xiiStringView sFileExtensions);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  xiiQtAssetBrowserModel*        GetAssetBrowserModel() { return m_pModel; }
  const xiiQtAssetBrowserModel*  GetAssetBrowserModel() const { return m_pModel; }
  xiiQtAssetBrowserFilter*       GetAssetBrowserFilter() { return m_pFilter; }
  const xiiQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
  void ItemSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
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
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(xiiUuid preselectedAsset);
  void OnScrollToFile(QString sPreselectedFile);
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
  void NewAsset();
  void OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset);
  void ImportSelection();
  void OnOpenImportReferenceAsset();
  void DeleteSelection();
  void OnImportAsAboutToShow();
  void OnImportAsClicked();


private:
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  void AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  void UpdateAssetTypes();
  void ProjectEventHandler(const xiiToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);
  void AddImportedViaMenu(QMenu* pMenu);
  void GetSelectedImportableFiles(xiiDynamicArray<xiiString>& out_Files) const;

  Mode                       m_Mode     = Mode::Browser;
  xiiQtToolBarActionMapView* m_pToolbar = nullptr;
  xiiString                  m_sAllTypesFilter;
  xiiQtAssetBrowserModel*    m_pModel  = nullptr;
  xiiQtAssetBrowserFilter*   m_pFilter = nullptr;

  /// \brief After creating a new asset and renaming it, we want to open it as well.
  bool m_bOpenAfterRename = false;
};
