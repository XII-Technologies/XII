/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <Foundation/Communication/Event.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtAssetBrowserModel;
class xiiQtAssetBrowserInspector;
class QAbstractItemView;
class QLabel;
class QAction;
class QListWidget;
class QMenu;
class QResizeEvent;
class QThreadPool;
class QTimer;
struct xiiAssetCuratorEvent;

/// Synchronous GUI-thread request. Providers may fill m_pPreview when they support this asset.
/// The returned widget belongs to m_pParent and must be safe to destroy on selection or project change.
struct xiiAssetBrowserPreviewRequest
{
  xiiUuid   m_Guid;
  xiiString m_sAssetType;
  QWidget*  m_pParent       = nullptr;
  bool      m_bQuickPreview = false;
  QWidget*  m_pPreview      = nullptr;
};

/// Contributes actions for selected asset identities. The menu and GUID view are borrowed for the callback.
struct xiiAssetBrowserContextMenu
{
  QMenu*                     m_pMenu = nullptr;
  xiiArrayPtr<const xiiUuid> m_Selection;
};

class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  xiiQtAssetBrowserWidget(QWidget* pParent);
  ~xiiQtAssetBrowserWidget();

  static xiiEvent<xiiAssetBrowserPreviewRequest&>    s_PreviewRequests;
  static xiiEvent<const xiiAssetBrowserContextMenu&> s_ContextMenuActions;

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
  void SetRequiredTag(xiiStringView sRequiredTag);

  void  SaveState(const char* szSettingsName);
  void  RestoreState(const char* szSettingsName);
  QSize minimumSizeHint() const override;

  void dragEnterEvent(QDragEnterEvent* pEvent) override;
  void dragMoveEvent(QDragMoveEvent* pEvent) override;
  void dragLeaveEvent(QDragLeaveEvent* pEvent) override;
  void dropEvent(QDropEvent* pEvent) override;

  xiiQtAssetBrowserModel*        GetAssetBrowserModel() { return m_pModel.data(); }
  const xiiQtAssetBrowserModel*  GetAssetBrowserModel() const { return m_pModel.data(); }
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
  void OnFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_ButtonDetailsMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(xiiInt32 iIconSizePercentage);
  void OnSearchWidgetTextChanged(const QString& text);
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(xiiUuid preselectedAsset);
  void OnScrollToFile(QString sPreselectedFile);
  void OnShowSubFolderItemsToggled();
  void OnShowHiddenFolderItemsToggled();
  void OnResaveAssets();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnListOpenFileWith();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnFilterToThisPath();
  void OnListFindAllReferences(bool transitive);
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void OnModelReset();
  void NewAsset();
  void OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset);
  void ImportSelection();
  void OnOpenImportReferenceAsset();
  void RenameCurrent();
  void DeleteSelection();
  void OnImportAsAboutToShow();
  void OnImportAsClicked();

private:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  void         resizeEvent(QResizeEvent* pEvent) override;

private:
  friend class xiiQtAssetBrowserInspector;

  void               AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  void               UpdateAssetTypes();
  void               ProjectEventHandler(const xiiToolsProjectEvent& e);
  void               AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset, bool bCreateSubmenu = true);
  void               AddImportedViaMenu(QMenu* pMenu);
  void               GetSelectedImportableFiles(xiiDynamicArray<xiiString>& out_Files) const;
  void               NavigateHistory(int iDirection);
  QAbstractItemView* ActiveView() const;
  QModelIndex        CurrentAssetIndex() const;
  QModelIndexList    SelectedAssets() const;
  void               SetDisplayMode(int iMode);
  void               UpdateWorkspace();
  void               ImportFiles();
  void               ImportUrls(const QList<QUrl>& urls);
  void               UpdateIndexStatus();
  void               UpdateFolderCounts();
  void               SaveProjectNavigation() const;
  void               RestoreProjectNavigation();
  void               ToggleFavorite(xiiUuid guid);
  void               ApplyStatusFilter(xiiQtAssetBrowserFilter::Status status, bool bGlobal);
  void               UpdatePanelVisibility();
  void               NavigateToAsset(xiiUuid guid);
  void               NavigateToFile(const QString& sPath);
  void               TransformSelection(bool bForce);
  void               DuplicateAsset(const QModelIndex& index);
  void               ShowInspectorTab(int iTab);

  struct NavigationLocation
  {
    QString                        m_sPath;
    xiiQtAssetBrowserFilter::Scope m_Scope = xiiQtAssetBrowserFilter::Scope::Content;

    bool operator==(const NavigationLocation& rhs) const = default;
  };

  Mode                                   m_Mode = Mode::Browser;
  xiiString                              m_sAllTypesFilter;
  QSharedPointer<xiiQtAssetBrowserModel> m_pModel  = nullptr;
  xiiQtAssetBrowserFilter*               m_pFilter = nullptr;

  /// After creating a new asset and renaming it, we want to open it as well.
  bool                        m_bOpenAfterRename   = false;
  QList<NavigationLocation>   m_PathHistory        = {NavigationLocation{}};
  xiiInt32                    m_iPathHistory       = 0;
  bool                        m_bHistoryNavigation = false;
  QAction*                    m_pBack              = nullptr;
  QAction*                    m_pForward           = nullptr;
  QAction*                    m_pUp                = nullptr;
  QLabel*                     m_pBreadcrumb        = nullptr;
  QMenu*                      m_pBreadcrumbMenu    = nullptr;
  QLabel*                     m_pVisibleCount      = nullptr;
  QLabel*                     m_pIndexStatus       = nullptr;
  QToolButton*                m_pPendingCount      = nullptr;
  QToolButton*                m_pErrorCount        = nullptr;
  QListWidget*                m_pScopes            = nullptr;
  QTimer*                     m_pSearchTimer       = nullptr;
  QTimer*                     m_pReferenceTimer    = nullptr;
  QTimer*                     m_pFolderCountTimer  = nullptr;
  QString                     m_sProjectSettings;
  int                         m_iDisplayMode     = 0;
  xiiQtAssetBrowserInspector* m_pInspector       = nullptr;
  QAction*                    m_pInspectorAction = nullptr;
  bool                        m_bShowSidebar     = true;
  bool                        m_bShowInspector   = true;
  bool                        m_bForceInspector  = false;
  QWidget*                    m_pStatusChips     = nullptr;
  QThreadPool*                m_pImportPool      = nullptr;
  bool                        m_bCopyingImport   = false;
};
