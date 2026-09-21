/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_AssetBrowserPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class QStatusBar;
class QLabel;
struct xiiToolsProjectEvent;
class xiiQtCuratorControl;

/// The application wide panel that shows and asset browser.
class XII_EDITORFRAMEWORK_DLL xiiQtAssetBrowserPanel : public xiiQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtAssetBrowserPanel);

public:
  xiiQtAssetBrowserPanel(ads::CDockManager* pDockManager);
  ~xiiQtAssetBrowserPanel();

  const xiiUuid& GetLastSelectedAsset() const { return m_LastSelected; }

private Q_SLOTS:
  void SlotAssetChosen(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
  void SlotAssetSelected(xiiUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, xiiUInt8 uiAssetBrowserItemFlags);
  void SlotAssetCleared();

private:
  void AssetCuratorEvents(const xiiAssetCuratorEvent& e);
  void ProjectEvents(const xiiToolsProjectEvent& e);

  xiiUuid              m_LastSelected;
  QStatusBar*          m_pStatusBar;
  xiiQtCuratorControl* m_pCuratorControl;
};
