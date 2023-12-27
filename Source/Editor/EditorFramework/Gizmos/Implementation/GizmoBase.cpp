#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Gizmos/GizmoBase.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGizmo::xiiGizmo()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
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
