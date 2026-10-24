/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGizmoHandle, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Visible", m_bVisible),
    XII_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEngineGizmoHandle, 2, xiiRTTIDefaultAllocator<xiiEngineGizmoHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HandleType", m_iHandleType),
    XII_MEMBER_PROPERTY("HandleMesh", m_sGizmoHandleMesh),
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    XII_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    XII_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
    XII_MEMBER_PROPERTY("Ortho", m_bShowInOrtho),
    XII_MEMBER_PROPERTY("Pickable", m_bIsPickable),
    XII_MEMBER_PROPERTY("FaceCam", m_bFaceCamera),
    XII_MEMBER_PROPERTY("VisualState", m_iVisualState),
    XII_MEMBER_PROPERTY("RotationArc", m_vArc),
    XII_MEMBER_PROPERTY("DragOrigin", m_vDragOrigin),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEngineGizmoHandle::xiiEngineGizmoHandle()  = default;
xiiEngineGizmoHandle::~xiiEngineGizmoHandle() = default;

void xiiGizmoHandle::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;
  m_bVisible = bVisible;
  SetModified();
}

void xiiGizmoHandle::SetTransformation(const xiiTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;
  m_Transformation = transform;
  SetModified();
}

void xiiGizmoHandle::SetTransformation(const xiiMat4& transform)
{
  SetTransformation(xiiTransform::MakeFromMat4(transform));
}

void xiiEngineGizmoHandle::ConfigureHandle(xiiGizmo* pOwner, xiiEngineGizmoHandleType type, const xiiColor& color, xiiBitflags<xiiGizmoFlags> flags, const char* szCustomMesh)
{
  m_pOwnerGizmo      = pOwner;
  m_iHandleType      = static_cast<xiiInt32>(type);
  m_sGizmoHandleMesh = szCustomMesh;
  m_Color            = color;
  m_bConstantSize    = flags.IsSet(xiiGizmoFlags::ConstantSize);
  m_bAlwaysOnTop     = flags.IsSet(xiiGizmoFlags::OnTop);
  m_bVisualizer      = flags.IsSet(xiiGizmoFlags::Visualizer);
  m_bShowInOrtho     = flags.IsSet(xiiGizmoFlags::ShowInOrtho);
  m_bIsPickable      = flags.IsSet(xiiGizmoFlags::Pickable);
  m_bFaceCamera      = flags.IsSet(xiiGizmoFlags::FaceCamera);
  m_bGeometryDirty   = true;
  SetModified();
}

void xiiEngineGizmoHandle::SetColor(const xiiColor& color)
{
  if (m_Color == color)
    return;
  m_Color = color;
  SetModified();
}

void xiiEngineGizmoHandle::SetVisualState(xiiGizmoVisualState state)
{
  if (m_iVisualState == static_cast<xiiInt32>(state))
    return;
  m_iVisualState = static_cast<xiiInt32>(state);
  SetModified();
}

void xiiEngineGizmoHandle::SetVisualParameters(float start, float delta)
{
  const xiiVec2 arc(start, delta);
  if (m_vArc == arc)
    return;
  m_vArc = arc;
  SetModified();
}

void xiiEngineGizmoHandle::SetDragOrigin(const xiiVec3& origin)
{
  if (m_vDragOrigin == origin)
    return;
  m_vDragOrigin = origin;
  SetModified();
}

bool xiiEngineGizmoHandle::SetupForEngine(xiiWorld*, xiiUInt32 uiNextComponentPickingID)
{
  if (m_uiPickingID != 0 || uiNextComponentPickingID == 0)
    return false;
  m_uiPickingID = uiNextComponentPickingID;
  return true;
}

void xiiEngineGizmoHandle::UpdateForEngine(xiiWorld*)
{
  m_bGeometryDirty |= m_GeometryResult.Failed() || m_iBuiltHandleType != m_iHandleType || m_sBuiltMesh != m_sGizmoHandleMesh;
}
