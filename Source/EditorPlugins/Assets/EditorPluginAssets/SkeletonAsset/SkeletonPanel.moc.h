#pragma once

#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class xiiSkeletonAssetDocument;
class QTreeView;
class xiiQtDocumentTreeView;
class xiiQtSearchWidget;

class xiiQtSkeletonPanel : public xiiQtDocumentPanel
{
  Q_OBJECT

public:
  xiiQtSkeletonPanel(ads::CDockManager* pDockManager, QWidget* pParent, xiiSkeletonAssetDocument* pDocument);
  ~xiiQtSkeletonPanel();

private:
  xiiSkeletonAssetDocument* m_pSkeletonDocument = nullptr;
  QWidget*                  m_pMainWidget       = nullptr;
  xiiQtDocumentTreeView*    m_pTreeWidget       = nullptr;
  xiiQtSearchWidget*        m_pFilterWidget     = nullptr;
};
