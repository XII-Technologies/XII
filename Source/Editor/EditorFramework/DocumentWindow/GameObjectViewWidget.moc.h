#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class xiiViewMarqueePickingResultMsgToEditor;
class xiiQtGameObjectDocumentWindow;
class xiiOrthoGizmoContext;
class xiiContextMenuContext;
class xiiSelectionContext;
class xiiCameraMoveContext;

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectViewWidget : public xiiQtEngineViewWidget
{
  Q_OBJECT
public:
  xiiQtGameObjectViewWidget(QWidget* pParent, xiiQtGameObjectDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig);
  ~xiiQtGameObjectViewWidget();

  xiiOrthoGizmoContext* m_pOrthoGizmoContext = nullptr;
  xiiSelectionContext*  m_pSelectionContext = nullptr;
  xiiCameraMoveContext* m_pCameraMoveContext = nullptr;

  virtual void SyncToEngine() override;

protected:
  virtual void HandleMarqueePickingResult(const xiiViewMarqueePickingResultMsgToEditor* pMsg) override;

  xiiUInt32         m_uiLastMarqueeActionID = 0;
  xiiDeque<xiiUuid> m_MarqueeBaseSelection;
};
