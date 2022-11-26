#pragma once

#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <Foundation/Basics.h>

class xiiQtSceneViewWidget : public xiiQtGameObjectViewWidget
{
  Q_OBJECT
public:
  xiiQtSceneViewWidget(QWidget* pParent, xiiQtGameObjectDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig);
  ~xiiQtSceneViewWidget();

  virtual bool IsPickingAgainstSelectionAllowed() const override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void OnOpenContextMenu(QPoint globalPos) override;

  bool    m_bAllowPickSelectedWhileDragging;
  xiiTime m_LastDragMoveEvent;

  static bool s_bContextMenuInitialized;
};
