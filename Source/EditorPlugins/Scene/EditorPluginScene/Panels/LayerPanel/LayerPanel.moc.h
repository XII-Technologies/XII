#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class xiiScene2Document;
class xiiQtLayerDelegate;

class xiiQtLayerPanel : public xiiQtDocumentPanel
{
  Q_OBJECT

public:
  xiiQtLayerPanel(QWidget* pParent, xiiScene2Document* pDocument);
  ~xiiQtLayerPanel();

private Q_SLOTS:
  void OnRequestContextMenu(QPoint pos);

private:
  xiiQtLayerDelegate*    m_pDelegate      = nullptr;
  xiiScene2Document*     m_pSceneDocument = nullptr;
  xiiQtDocumentTreeView* m_pTreeWidget    = nullptr;
  xiiString              m_sContextMenuMapping;
};
