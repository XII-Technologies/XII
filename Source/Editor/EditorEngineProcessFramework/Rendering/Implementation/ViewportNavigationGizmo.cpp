/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Rendering/ViewportNavigationGizmo.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

xiiVec3 xiiViewportNavigationGizmo::GetAxisDirection(xiiUInt32 uiAxis)
{
  XII_ASSERT_DEV(uiAxis < 6, "Invalid navigation axis.");
  xiiVec3 axis               = xiiVec3::MakeZero();
  axis.GetData()[uiAxis / 2] = (uiAxis & 1) ? -1.0f : 1.0f;
  return axis;
}

xiiViewportNavigationGizmo::xiiViewportNavigationGizmo(const xiiCamera& camera, const xiiVec2& vViewportSize)
{
  constexpr float size = 112.0f;
  m_Bounds             = xiiRectFloat(xiiMath::Max(0.0f, vViewportSize.x - size - 8.0f), 8.0f, size, size);
  m_vOrigin            = xiiVec2(m_Bounds.x + size * 0.5f, m_Bounds.y + size * 0.5f);
  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    const xiiVec3 axis = GetAxisDirection(i);
    // Dot products with the camera basis are its inverse orientation. Translation is irrelevant.
    m_Markers[i] = {m_vOrigin + xiiVec2(axis.Dot(camera.GetCenterDirRight()), -axis.Dot(camera.GetCenterDirUp())) * 38.0f,
                    -axis.Dot(camera.GetCenterDirForwards()), (i & 1) ? 7.0f : 10.0f, i};
  }
  // Stable insertion sort gives deterministic ordering for overlapping coplanar markers.
  for (xiiUInt32 i = 1; i < 6; ++i)
  {
    const Marker marker = m_Markers[i];
    xiiUInt32    j      = i;
    while (j > 0 && m_Markers[j - 1].m_fDepth > marker.m_fDepth)
    {
      m_Markers[j] = m_Markers[j - 1];
      --j;
    }
    m_Markers[j] = marker;
  }
}

bool xiiViewportNavigationGizmo::Contains(const xiiVec2& vPosition) const
{
  return (vPosition - m_vOrigin).GetLengthSquared() <= 54.0f * 54.0f;
}

xiiInt32 xiiViewportNavigationGizmo::HitTest(const xiiVec2& vPosition) const
{
  for (xiiUInt32 i = 6; i > 0; --i)
  {
    const auto& marker = m_Markers[i - 1];
    if ((vPosition - marker.m_vPosition).GetLengthSquared() <= xiiMath::Square(marker.m_fRadius + 2.0f))
      return static_cast<xiiInt32>(marker.m_uiAxis);
  }
  return -1;
}

void xiiViewportNavigationGizmo::Draw(const xiiDebugRendererContext& context, float fDevicePixelRatio, xiiInt32 iHoveredAxis, bool bHovered) const
{
  const xiiColor colors[] = {xiiColor(0.9f, 0.12f, 0.16f), xiiColor(0.25f, 0.8f, 0.12f), xiiColor(0.15f, 0.4f, 1.0f)};
  const char*    labels[] = {"X", "Y", "Z"};
  const xiiVec2  origin   = m_vOrigin * fDevicePixelRatio;
  if (bHovered)
    xiiDebugRenderer::Draw2DCircle(context, origin, 54.0f * fDevicePixelRatio, xiiColor(0.08f, 0.08f, 0.08f, 0.65f));

  for (const auto& marker : m_Markers)
  {
    const xiiVec2 position = marker.m_vPosition * fDevicePixelRatio;
    xiiColor      color    = colors[marker.m_uiAxis / 2];
    if (marker.m_fDepth < -0.001f)
      color *= 0.45f;
    color.a = 1.0f;
    const xiiDebugRendererLine line(xiiVec3(origin.x, origin.y, 1), xiiVec3(position.x, position.y, 1));
    xiiDebugRenderer::Draw2DLines(context, xiiMakeArrayPtr(&line, 1), color, xiiDebugRendererDepthMode::Overlay);
    if (iHoveredAxis == static_cast<xiiInt32>(marker.m_uiAxis))
      xiiDebugRenderer::Draw2DCircle(context, position, (marker.m_fRadius + 2.5f) * fDevicePixelRatio, xiiColor::White);
    xiiDebugRenderer::Draw2DCircle(context, position, marker.m_fRadius * fDevicePixelRatio, color);
    if (marker.m_uiAxis & 1)
      xiiDebugRenderer::Draw2DCircle(context, position, (marker.m_fRadius - 2.0f) * fDevicePixelRatio, xiiColor(0.08f, 0.08f, 0.08f));
    else if (HitTest(marker.m_vPosition) == static_cast<xiiInt32>(marker.m_uiAxis))
      xiiDebugRenderer::Draw2DText(context, labels[marker.m_uiAxis / 2], xiiVec2I32(xiiMath::RoundToInt(position.x), xiiMath::RoundToInt(position.y)), xiiColor::White,
                                   xiiMath::Max(1U, static_cast<xiiUInt32>(13.0f * fDevicePixelRatio)), xiiDebugTextHAlign::Center, xiiDebugTextVAlign::Center);
  }
}
