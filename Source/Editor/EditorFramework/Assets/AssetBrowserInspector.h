/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Types/Uuid.h>

#include <QPersistentModelIndex>
#include <QPointer>
#include <QWidget>

class QCheckBox;
class QDialog;
class QLabel;
class QPlainTextEdit;
class QResizeEvent;
class QTabWidget;
class QToolButton;
class QTreeWidget;
class xiiQtAssetBrowserWidget;

/// Browser-owned selection inspector. Only persistent model indexes and value snapshots survive a refresh.
class xiiQtAssetBrowserInspector final : public QWidget
{
public:
  xiiQtAssetBrowserInspector(xiiQtAssetBrowserWidget* pBrowser, QWidget* pParent);

  void SetSelection(const QModelIndexList& selection);
  void RefreshAsset();
  void RefreshPreview();
  void RefreshFileSize();
  void RefreshProcessingStatus();
  enum Tab
  {
    Overview,
    Dependencies,
    Diagnostics
  };
  void ShowTab(int iTab);
  void ShowQuickPreview();
  void Clear();

protected:
  void resizeEvent(QResizeEvent* pEvent) override;

private:
  QWidget* CreateSpecializedPreview(QWidget* pParent, bool bQuickPreview);
  void     RefreshDependencies();

  xiiQtAssetBrowserWidget* m_pBrowser;
  QPersistentModelIndex    m_Selected;
  QTabWidget*              m_pTabs;
  QLabel*                  m_pSummary;
  QLabel*                  m_pPreview = nullptr;
  QWidget*                 m_pOverview;
  QPointer<QWidget>        m_pSpecializedPreview;
  QLabel*                  m_pStatus;
  QTreeWidget*             m_pDetails;
  QTreeWidget*             m_pDependencies;
  QCheckBox*               m_pTransitive;
  QPlainTextEdit*          m_pDiagnostics;
  QToolButton*             m_pOpen;
  QToolButton*             m_pTransform;
  QToolButton*             m_pForceTransform;
  QToolButton*             m_pQuickPreviewButton;
  bool                     m_bDependenciesDirty = true;
  QPointer<QDialog>        m_pQuickPreview;
  QPointer<QLabel>         m_pQuickPreviewImage;
};
