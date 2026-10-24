/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Gizmos/GizmoBase.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGizmo::xiiGizmo()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
  m_GizmoEvents.AddEventHandler([this](const xiiGizmoEvent& event) {
    auto* handle = xiiDynamicCast<xiiEngineGizmoHandle*>(m_pInteractionGizmoHandle);
    if (!handle)
      return;
    if (event.m_Type == xiiGizmoEvent::Type::BeginInteractions)
    {
      handle->SetDragOrigin(GetTransformation().m_vPosition);
      handle->SetVisualState(xiiGizmoVisualState::Active);
    }
    else if (event.m_Type == xiiGizmoEvent::Type::EndInteractions || event.m_Type == xiiGizmoEvent::Type::CancelInteractions)
    {
      handle->SetVisualState(xiiGizmoVisualState::Normal);
      handle->SetVisualParameters(0, 0);
      handle->SetInteractionText({});
    }
  });
}

void xiiGizmo::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void xiiGizmo::SetTransformation(const xiiTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}
