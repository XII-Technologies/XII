/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
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
  if (!m_bVisible || !camera || !m_Transformation.IsValid() || !m_Color.IsValid() ||
      !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 ||
      camera->GetCameraMode() == xiiCameraMode::None || (camera->IsOrthographic() && !m_bShowInOrtho))
    return XII_FAILURE;

  const float fProjectionScale = xiiMath::Abs(view.GetProjectionMatrix(xiiCameraEye::Left).Element(1, 1));
  const float fDepth           = (m_Transformation.m_vPosition - camera->GetCenterPosition()).Dot(camera->GetCenterDirForwards());
  if (!xiiMath::IsFinite(fProjectionScale) || fProjectionScale <= 0 || (!camera->IsOrthographic() && fDepth <= 0))
    return XII_FAILURE;
  const float fWorldUnitsPerPixel = 2.0f * (camera->IsOrthographic() ? 1.0f : fDepth) / (fProjectionScale * viewport.height);
  if (!xiiMath::IsFinite(fWorldUnitsPerPixel) || fWorldUnitsPerPixel <= 0)
    return XII_FAILURE;

  xiiMat4 transform = m_Transformation.GetAsMat4();
  xiiVec3 scale(m_bConstantSize ? 128.0f * s_fGizmoScale * fWorldUnitsPerPixel : 1.0f);
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
  m_bBuiltVisualizer = m_bVisualizer;
  m_pWireGeometry.Clear();
  if (m_iHandleType < 0 || m_iHandleType > static_cast<xiiInt32>(xiiEngineGizmoHandleType::FromFile))
    return XII_FAILURE;
  if (!m_pGeometry)
    m_pGeometry = XII_DEFAULT_NEW(xiiGizmoGeometry);
  XII_SUCCEED_OR_RETURN(m_pGeometry->Build(static_cast<xiiEngineGizmoHandleType>(m_iHandleType), m_sGizmoHandleMesh));
  if (m_bVisualizer)
  {
    m_pWireGeometry = XII_DEFAULT_NEW(xiiGeometry, m_pGeometry->GetGeometry());
    xiiHashSet<xiiUInt64> edges;
    for (const auto& line : m_pWireGeometry->GetLines())
      edges.Insert((static_cast<xiiUInt64>(xiiMath::Min(line.m_uiStartVertex, line.m_uiEndVertex)) << 32) | xiiMath::Max(line.m_uiStartVertex, line.m_uiEndVertex));
    for (const auto& triangle : m_pWireGeometry->GetPolygons())
    {
      for (xiiUInt32 i = 0; i < 3; ++i)
      {
        const xiiUInt32 a = triangle.m_Vertices[i];
        const xiiUInt32 b = triangle.m_Vertices[(i + 1) % 3];
        if (!edges.Insert((static_cast<xiiUInt64>(xiiMath::Min(a, b)) << 32) | xiiMath::Max(a, b)))
          m_pWireGeometry->AddLine(a, b);
      }
    }
    m_pWireGeometry->GetPolygons().Clear();
  }
  m_GeometryResult = XII_SUCCESS;
  return XII_SUCCESS;
}

xiiResult xiiEngineGizmoHandle::Draw(const xiiView& view, xiiUInt32 uiHighlightID)
{
  if (!m_bVisible || (view.GetCamera() && view.GetCamera()->IsOrthographic() && !m_bShowInOrtho))
    return XII_SUCCESS;
  if (view.GetCamera() && !view.GetCamera()->IsOrthographic() && m_Transformation.IsValid() &&
      (m_Transformation.m_vPosition - view.GetCamera()->GetCenterPosition()).Dot(view.GetCamera()->GetCenterDirForwards()) <= 0)
    return XII_SUCCESS;
  xiiMat4 transform;
  float   fPixelSize;
  XII_SUCCEED_OR_RETURN(ComputeViewTransform(view, transform, fPixelSize));
  XII_SUCCEED_OR_RETURN(EnsureGeometry());
  const xiiColor color = m_uiPickingID != 0 && m_uiPickingID == uiHighlightID ? xiiColor(0.9f, 0.9f, 0.1f, m_Color.a) : m_Color;
  return xiiDebugRenderer::DrawGeometry(view, m_bVisualizer ? *m_pWireGeometry : m_pGeometry->GetGeometry(), color, transform,
                                        m_bAlwaysOnTop ? xiiDebugRendererDepthMode::Overlay : xiiDebugRendererDepthMode::DepthTested);
}

xiiResult xiiEngineGizmoHandle::PickRay(const xiiView& view, const xiiVec3& vStart, const xiiVec3& vDirection, float& out_fRayTime)
{
  if (!m_bIsPickable || m_bVisualizer || !m_Color.IsValid())
    return XII_FAILURE;
  xiiMat4 transform;
  float   fPixelSize;
  XII_SUCCEED_OR_RETURN(ComputeViewTransform(view, transform, fPixelSize));
  XII_SUCCEED_OR_RETURN(EnsureGeometry());
  return m_pGeometry->PickRay(transform, vStart, vDirection, 4.0f * fPixelSize, out_fRayTime);
}
