#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class xiiQtSearchWidget;
class xiiGameObjectDocument;
struct xiiGameObjectEvent;

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectWidget : public QWidget
{
  Q_OBJECT

public:
  xiiQtGameObjectWidget(QWidget* pParent, xiiGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel, xiiSelectionManager* pSelection = nullptr);
  ~xiiQtGameObjectWidget();

private Q_SLOTS:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const xiiGameObjectEvent& e);

protected:
  xiiGameObjectDocument* m_pDocument     = nullptr;
  xiiQtDocumentTreeView* m_pTreeWidget   = nullptr;
  xiiQtSearchWidget*     m_pFilterWidget = nullptr;
  xiiString              m_sContextMenuMapping;
};

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectPanel : public xiiQtDocumentPanel
{
  Q_OBJECT

public:
  xiiQtGameObjectPanel(QWidget* pParent, xiiGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<xiiQtDocumentTreeModel> pCustomModel);
  ~xiiQtGameObjectPanel();


protected:
  xiiQtGameObjectWidget* m_pMainWidget = nullptr;
};
