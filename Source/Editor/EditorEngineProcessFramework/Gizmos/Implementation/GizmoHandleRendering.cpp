/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorEngineProcessFramework/Rendering/Implementation/EditorViewportUtils.h>


#include <GraphicsCore/Pipeline/View.h>

namespace
{
  float s_fGizmoScale = 1.0f;
}

xiiResult xiiEngineGizmoHandle::SetGizmoScale(float fScale)
{
  if (!xiiMath::IsFinite(fScale) || fScale <= 0)
    return XII_FAILURE;
  s_fGizmoScale = fScale;
  return XII_SUCCESS;
}

float xiiEngineGizmoHandle::GetGizmoScale()
{
  return s_fGizmoScale;
}

xiiResult xiiEngineGizmoHandle::ComputeViewTransform(const xiiView& view, xiiMat4& out_transform, float& out_fWorldUnitsPerPixel) const
{
  const auto* camera   = view.GetCamera();
  const auto& viewport = view.GetViewport();
  if (!m_bVisible || !camera || !m_Transformation.IsValid() || !m_Color.IsValid() || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 || camera->GetCameraMode() == xiiCameraMode::None || (camera->IsOrthographic() && !m_bShowInOrtho))
    return XII_FAILURE;

  const float fProjectionScale = xiiMath::Abs(view.GetProjectionMatrix(xiiCameraEye::Left).Element(1, 1));
  const float fDepth           = (m_Transformation.m_vPosition - camera->GetCenterPosition()).Dot(camera->GetCenterDirForwards());
  if (!xiiMath::IsFinite(fProjectionScale) || fProjectionScale <= 0 || (!camera->IsOrthographic() && fDepth <= 0))
    return XII_FAILURE;
  const float fWorldUnitsPerPixel = 2.0f * (camera->IsOrthographic() ? 1.0f : fDepth) / (fProjectionScale * viewport.height);
  if (!xiiMath::IsFinite(fWorldUnitsPerPixel) || fWorldUnitsPerPixel <= 0)
    return XII_FAILURE;

  xiiMat4 transform = m_Transformation.GetAsMat4();
  float   ratio;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::PixelRatio(view, ratio));
  xiiVec3 scale(m_bConstantSize ? 128.0f * ratio * s_fGizmoScale * fWorldUnitsPerPixel : 1.0f);
  if (m_bFaceCamera)
  {
    xiiMat4 inverse = transform;
    XII_SUCCEED_OR_RETURN(inverse.Invert(0.0f));
    if (!inverse.IsValid())
      return XII_FAILURE;
    const xiiVec3 localCamera = camera->IsOrthographic() ? inverse.TransformDirection(-camera->GetCenterDirForwards()) : inverse.TransformPosition(camera->GetCenterPosition());
    for (xiiUInt32 axis = 0; axis < 3; ++axis)
      if (localCamera.GetData()[axis] < 0)
        scale.GetData()[axis] = -scale.GetData()[axis];
  }
  transform = transform * xiiMat4::MakeScaling(scale);
  if (!transform.IsValid())
    return XII_FAILURE;
  const float fDeterminant = transform.GetRotationalPart().GetDeterminant();
  if (!xiiMath::IsFinite(fDeterminant) || fDeterminant == 0.0f)
    return XII_FAILURE;
  out_transform           = transform;
  out_fWorldUnitsPerPixel = fWorldUnitsPerPixel;
  return XII_SUCCESS;
}

xiiResult xiiEngineGizmoHandle::EnsureGeometry()
{
  if (!m_bGeometryDirty)
    return m_GeometryResult;
  m_bGeometryDirty   = false;
  m_GeometryResult   = XII_FAILURE;
  m_iBuiltHandleType = m_iHandleType;
  m_sBuiltMesh       = m_sGizmoHandleMesh;


  if (m_iHandleType < 0 || m_iHandleType > static_cast<xiiInt32>(xiiEngineGizmoHandleType::ScaleUniform))
    return XII_FAILURE;
  if (!m_pGeometry)
    m_pGeometry = XII_DEFAULT_NEW(xiiGizmoGeometry);
  XII_SUCCEED_OR_RETURN(m_pGeometry->Build(static_cast<xiiEngineGizmoHandleType>(m_iHandleType), m_sGizmoHandleMesh));
  m_GeometryResult = XII_SUCCESS;
  return XII_SUCCESS;
}

xiiResult xiiEngineGizmoHandle::AppendRenderData(const xiiView& view, xiiUInt32 highlight, xiiDynamicArray<xiiEditorGizmoInstance>& instances, xiiVariantArray& customMeshes) const
{
  if (!m_bVisible || m_Color.a <= 0 || (view.GetCamera() && view.GetCamera()->IsOrthographic() && !m_bShowInOrtho))
    return XII_SUCCESS;
  if (view.GetCamera() && !view.GetCamera()->IsOrthographic() && m_Transformation.IsValid() && (m_Transformation.m_vPosition - view.GetCamera()->GetCenterPosition()).Dot(view.GetCamera()->GetCenterDirForwards()) <= 0)
    return XII_SUCCESS;
  if (m_iHandleType < 0 || m_iHandleType > static_cast<xiiInt32>(xiiEngineGizmoHandleType::ScaleUniform) || m_iVisualState < 0 || m_iVisualState > static_cast<xiiInt32>(xiiGizmoVisualState::Disabled) || !m_vArc.IsValid() || !m_vDragOrigin.IsValid())
    return XII_FAILURE;
  xiiMat4 transform;
  float   pixel;
  XII_SUCCEED_OR_RETURN(ComputeViewTransform(view, transform, pixel));
  const auto camera   = view.GetCamera()->GetCenterPosition();
  const auto position = transform.GetTranslationVector();
  transform.SetTranslationVector(xiiVec3(float(double(position.x) - camera.x), float(double(position.y) - camera.y), float(double(position.z) - camera.z)));
  xiiEditorGizmoInstance instance = {};
  instance.Transform              = transform;
  instance.Color                  = xiiVec4(m_Color.r, m_Color.g, m_Color.b, m_Color.a);
  xiiUInt32 flags                 = m_bAlwaysOnTop ? XII_EDITOR_GIZMO_ON_TOP : 0;
  if (m_bVisualizer)
    flags |= XII_EDITOR_GIZMO_WIRE;
  if (m_iVisualState == static_cast<xiiInt32>(xiiGizmoVisualState::Active))
    flags |= XII_EDITOR_GIZMO_ACTIVE;
  if (m_iVisualState == static_cast<xiiInt32>(xiiGizmoVisualState::Selected))
    flags |= XII_EDITOR_GIZMO_SELECTED;
  if (m_iVisualState == static_cast<xiiInt32>(xiiGizmoVisualState::Disabled))
    flags |= XII_EDITOR_GIZMO_DISABLED;
  if (m_uiPickingID && m_uiPickingID == highlight)
    flags |= XII_EDITOR_GIZMO_HOVER;
  instance.Identity   = xiiVec4U32(m_iHandleType, flags, m_uiPickingID, 0);
  instance.GuideStart = xiiVec4(float(double(m_vDragOrigin.x) - camera.x), float(double(m_vDragOrigin.y) - camera.y), float(double(m_vDragOrigin.z) - camera.z), 0);
  instance.Parameters = xiiVec4(flags & XII_EDITOR_GIZMO_ACTIVE ? 2.8f : flags & XII_EDITOR_GIZMO_HOVER ? 2.5f :
                                                                                                          2.0f,
                                m_vArc.x, m_vArc.y, 1);
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::PixelRatio(view, instance.Parameters.w));
  instances.PushBack(instance);
  customMeshes.PushBack(m_sGizmoHandleMesh);
  return XII_SUCCESS;
}

xiiResult xiiEngineGizmoHandle::PickRay(const xiiView& view, const xiiVec3& vStart, const xiiVec3& vDirection, float& out_fRayTime)
{
  if (!m_bIsPickable || m_bVisualizer || !m_Color.IsValid() || m_Color.a <= 0 || m_iVisualState == static_cast<xiiInt32>(xiiGizmoVisualState::Disabled))
    return XII_FAILURE;
  xiiMat4 transform;
  float   fPixelSize;
  XII_SUCCEED_OR_RETURN(ComputeViewTransform(view, transform, fPixelSize));
  XII_SUCCEED_OR_RETURN(EnsureGeometry());
  float ratio;
  XII_SUCCEED_OR_RETURN(xiiEditorViewportUtils::PixelRatio(view, ratio));
  return m_pGeometry->PickRay(transform, vStart, vDirection, 4.0f * ratio * fPixelSize, out_fRayTime);
}
