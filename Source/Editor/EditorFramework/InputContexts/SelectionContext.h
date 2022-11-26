#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class QWidget;
class xiiCamera;
struct xiiObjectPickingResult;
class xiiDocumentObject;

class XII_EDITORFRAMEWORK_DLL xiiSelectionContext : public xiiEditorInputContext
{
public:
  xiiSelectionContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera);

  void SetWindowConfig(const xiiVec2I32& viewport) { m_vViewport = viewport; }

  void SetPickObjectOverride(xiiDelegate<void(const xiiDocumentObject*)> pickOverride);
  void ResetPickObjectOverride();

protected:
  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual xiiEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override {}

  const xiiDocumentObject* determineObjectToSelect(const xiiDocumentObject* pickedObject, bool bToggle, bool bDirect) const;

  virtual void DoFocusLost(bool bCancel) override;

  virtual void OpenDocumentForPickedObject(const xiiObjectPickingResult& res) const;
  virtual void SelectPickedObject(const xiiObjectPickingResult& res, bool bToggle, bool bDirect) const;

protected:
  void SendMarqueeMsg(QMouseEvent* e, xiiUInt8 uiWhatToDo);

  xiiDelegate<void(const xiiDocumentObject*)> m_PickObjectOverride;
  const xiiCamera*                            m_pCamera;
  xiiVec2I32                                  m_vViewport;
  xiiEngineGizmoHandle                        m_hMarqueeGizmo;
  xiiVec3                                     m_vMarqueeStartPos;
  xiiUInt32                                   m_uiMarqueeID;
  bool                                        m_bPressedSpace = false;

  enum class Mode
  {
    None,
    Single,
    MarqueeAdd,
    MarqueeRemove
  };

  Mode m_Mode = Mode::None;
};
