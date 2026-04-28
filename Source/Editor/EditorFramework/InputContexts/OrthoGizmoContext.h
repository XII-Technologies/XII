/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

#include <QPoint>

class QWidget;
class xiiCamera;

class XII_EDITORFRAMEWORK_DLL xiiOrthoGizmoContext : public xiiEditorInputContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOrthoGizmoContext, xiiEditorInputContext);

public:
  xiiOrthoGizmoContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera);

  void SetWindowConfig(const xiiVec2I32& vViewport) { m_vViewport = vViewport; }

  virtual void FocusLost(bool bCancel);

  xiiEvent<const xiiGizmoEvent&> m_GizmoEvents;

  const xiiVec3& GetTranslationResult() const { return m_vTranslationResult; }
  const xiiVec3& GetTranslationDiff() const { return m_vTranslationDiff; }
  const xiiQuat& GetRotationResult() const { return m_qRotationResult; }
  float          GetScalingResult() const { return m_fScalingResult; }

protected:
  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override {}

private:
  bool IsViewInOrthoMode() const;

  xiiVec2I32       m_vLastMousePos;
  xiiVec3          m_vUnsnappedTranslationResult;
  xiiVec3          m_vTranslationResult;
  xiiVec3          m_vTranslationDiff;
  xiiAngle         m_UnsnappedRotationResult;
  xiiQuat          m_qRotationResult;
  float            m_fScaleMouseMove;
  float            m_fScalingResult;
  float            m_fUnsnappedScalingResult;
  bool             m_bCanInteract;
  const xiiCamera* m_pCamera;
  xiiVec2I32       m_vViewport;
};
