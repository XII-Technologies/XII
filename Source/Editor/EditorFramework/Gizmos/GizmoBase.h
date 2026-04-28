/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Logging/Log.h>

class xiiCamera;

struct xiiGizmoEvent
{
  enum class Type
  {
    BeginInteractions,
    EndInteractions,
    Interaction,
    CancelInteractions,
  };

  const xiiEditorInputContext* m_pGizmo = nullptr;
  Type                         m_Type;
};

class XII_EDITORFRAMEWORK_DLL xiiGizmo : public xiiEditorInputContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmo, xiiEditorInputContext);

public:
  xiiGizmo();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void                SetTransformation(const xiiTransform& transform);
  const xiiTransform& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(xiiGizmoHandle* pHandle, const xiiCamera* pCamera, const xiiVec3& vInteractionPivot, const xiiVec2I32& vViewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera                 = pCamera;
    m_vInteractionPivot       = vInteractionPivot;
    m_vViewport               = vViewport;
  }

  xiiEvent<const xiiGizmoEvent&> m_GizmoEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible)                        = 0;
  virtual void OnTransformationChanged(const xiiTransform& transform) = 0;

  const xiiCamera* m_pCamera;
  xiiGizmoHandle*  m_pInteractionGizmoHandle;
  xiiVec3          m_vInteractionPivot;
  xiiVec2I32       m_vViewport;

private:
  bool         m_bVisible;
  xiiTransform m_Transformation;
};
