#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/ui_AssetCuratorPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class xiiQtCuratorControl;
struct xiiLoggingEventData;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetCuratorFilter : public xiiQtAssetFilter
{
  Q_OBJECT
public:
  explicit xiiQtAssetCuratorFilter(QObject* pParent);

  void SetFilterTransitive(bool bFilterTransitive);

public:
  virtual bool IsAssetFiltered(xiiStringView sDataDirParentRelativePath, bool bIsFolder, const xiiSubAsset* pInfo) const override;

  bool m_bFilterTransitive = true;
};

class XII_EDITORFRAMEWORK_DLL xiiQtAssetCuratorPanel : public xiiQtApplicationPanel, public Ui_AssetCuratorPanel
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtAssetCuratorPanel);

public:
  xiiQtAssetCuratorPanel(ads::CDockManager* pDockManager);
  ~xiiQtAssetCuratorPanel();

public Q_SLOTS:
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private Q_SLOTS:
  // note, because of the way we set up the widget, auto-connect doesn't work
  void onListAssetsDoubleClicked(const QModelIndex& index);
  void onCheckIndirectToggled(bool checked);

private:
  void LogWriter(const xiiLoggingEventData& e);
  void UpdateIssueInfo();

  QSharedPointer<xiiQtAssetBrowserModel>  m_pModel;
  xiiQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex    m_SelectedIndex;
};
