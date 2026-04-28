/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QSortFilterProxyModel>
#include <ToolsFoundation/Selection/SelectionManager.h>

#include <QTreeView>
#include <memory>

class xiiQtTreeSearchFilterModel;
class xiiSelectionManager;

class XII_EDITORFRAMEWORK_DLL xiiQtDocumentTreeView : public xiiQtItemView<QTreeView>
{
  Q_OBJECT

public:
  xiiQtDocumentTreeView(QWidget* pParent);
  xiiQtDocumentTreeView(QWidget* pParent, xiiDocument* pDocument, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel, xiiSelectionManager* pSelection = nullptr);
  ~xiiQtDocumentTreeView();

  void Initialize(xiiDocument* pDocument, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel, xiiSelectionManager* pSelection = nullptr);

  void EnsureLastSelectedItemVisible();

  void SetAllowDragDrop(bool bAllow);
  void SetAllowDeleteObjects(bool bAllow);

  xiiQtTreeSearchFilterModel* GetProxyFilterModel() const { return m_pFilterModel.get(); }

protected:
  virtual bool event(QEvent* pEvent) override;

private Q_SLOTS:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

private:
  std::unique_ptr<xiiQtDocumentTreeModel>     m_pModel;
  std::unique_ptr<xiiQtTreeSearchFilterModel> m_pFilterModel;
  xiiSelectionManager*                        m_pSelectionManager     = nullptr;
  xiiDocument*                                m_pDocument             = nullptr;
  bool                                        m_bBlockSelectionSignal = false;
  bool                                        m_bAllowDeleteObjects   = false;
};
