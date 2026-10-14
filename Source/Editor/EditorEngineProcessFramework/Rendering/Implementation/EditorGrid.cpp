/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Rendering/EditorGrid.h>
#include <Foundation/Math/ColorScheme.h>
#include <GraphicsCore/Pipeline/View.h>

xiiResult xiiEditorGrid::Build(const xiiView& view, float fSpacing, const xiiTransform& transform, bool bGlobal)
{
  m_Lines.Clear();
  if (!xiiMath::IsFinite(fSpacing) || fSpacing < 0 || !transform.IsValid())
    return XII_FAILURE;
  if (fSpacing == 0 || transform.m_vScale.IsZero(0.0f))
    return XII_SUCCESS;
  const auto* camera   = view.GetCamera();
  const auto& viewport = view.GetViewport();
  if (!camera || camera->GetCameraMode() == xiiCameraMode::None || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0)
    return XII_FAILURE;

  xiiVec3    center   = transform.m_vPosition;
  xiiVec3    tangent1 = transform.m_qRotation * xiiVec3::MakeAxisX();
  xiiVec3    tangent2 = transform.m_qRotation * xiiVec3::MakeAxisY();
  double     spacing  = fSpacing;
  double     first1 = -50, last1 = 50, first2 = -50, last2 = 50;
  const bool bOrtho = camera->IsOrthographic();
  if (bOrtho)
  {
    const float aspect = viewport.width / viewport.height;
    if (!xiiMath::IsFinite(aspect) || aspect <= 0)
      return XII_FAILURE;
    const double halfWidth  = static_cast<double>(camera->GetDimensionX(aspect)) * 0.5;
    const double halfHeight = static_cast<double>(camera->GetDimensionY(aspect)) * 0.5;
    const double depth      = static_cast<double>(camera->GetNearPlane()) * 0.05 + static_cast<double>(camera->GetFarPlane()) * 0.95;
    if (!xiiMath::IsFinite(halfWidth) || !xiiMath::IsFinite(halfHeight) || halfWidth <= 0 || halfHeight <= 0 || !xiiMath::IsFinite(depth))
      return XII_FAILURE;
    // Compute in double precision so tiny snap steps cannot overflow an integer density multiplier.
    const double minimumSpacing = xiiMath::Max(xiiMath::Max(20.0 * halfWidth / viewport.width, 20.0 * halfHeight / viewport.height), xiiMath::Max(halfWidth, halfHeight) / 1024.0);
    while (spacing < minimumSpacing)
      spacing *= 10.0;
    tangent1                     = camera->GetCenterDirRight();
    tangent2                     = camera->GetCenterDirUp();
    const xiiVec3 origin         = bGlobal ? xiiVec3::MakeZero() : transform.m_vPosition;
    const xiiVec3 relativeCamera = camera->GetCenterPosition() - origin;
    if (!relativeCamera.IsValid() || !tangent1.IsValid() || !tangent2.IsValid())
      return XII_FAILURE;
    const double position1 = relativeCamera.Dot(tangent1);
    const double position2 = relativeCamera.Dot(tangent2);
    first1                 = xiiMath::Floor((position1 - halfWidth) / spacing);
    last1                  = xiiMath::Ceil((position1 + halfWidth) / spacing);
    first2                 = xiiMath::Floor((position2 - halfHeight) / spacing);
    last2                  = xiiMath::Ceil((position2 + halfHeight) / spacing);
    // Form the plane in double precision to reject overflow before it reaches vector arithmetic.
    const auto cameraPosition = camera->GetCenterPosition();
    const auto forward        = camera->GetCenterDirForwards();
    for (xiiUInt32 axis = 0; axis < 3; ++axis)
      center.GetData()[axis] = static_cast<float>(static_cast<double>(cameraPosition.GetData()[axis]) + forward.GetData()[axis] * depth - tangent1.GetData()[axis] * position1 - tangent2.GetData()[axis] * position2);
  }
  if (!center.IsValid() || !tangent1.IsValid() || !tangent2.IsValid() || !xiiMath::IsFinite(spacing) || spacing > xiiMath::MaxValue<float>() || !xiiMath::IsFinite(first1) || !xiiMath::IsFinite(last1) || !xiiMath::IsFinite(first2) || !xiiMath::IsFinite(last2) || xiiMath::Abs(first1) > 1.0e15 || xiiMath::Abs(last1) > 1.0e15 || xiiMath::Abs(first2) > 1.0e15 || xiiMath::Abs(last2) > 1.0e15)
    return XII_FAILURE;
  if (xiiMath::Max(xiiMath::Max(xiiMath::Abs(first1), xiiMath::Abs(last1)), xiiMath::Max(xiiMath::Abs(first2), xiiMath::Abs(last2))) * spacing > xiiMath::MaxValue<float>() * 0.5)
    return XII_FAILURE;

  const xiiInt64 start1 = static_cast<xiiInt64>(first1), end1 = static_cast<xiiInt64>(last1);
  const xiiInt64 start2 = static_cast<xiiInt64>(first2), end2 = static_cast<xiiInt64>(last2);
  if (end1 - start1 > 2050 || end2 - start2 > 2050)
    return XII_FAILURE;
  xiiColor centerColor = xiiColorScheme::GetColor(xiiColorScheme::Blue, 6, 0.5f);
  xiiColor majorColor  = xiiColorScheme::LightUI(xiiColorScheme::Gray) * 0.7f;
  xiiColor minorColor  = xiiColorScheme::LightUI(xiiColorScheme::Gray) * 0.5f;
  if (bOrtho)
  {
    centerColor *= 0.5f;
    majorColor *= 0.4f;
    minorColor *= 0.4f;
  }
  if (bGlobal != bOrtho)
    centerColor = majorColor;
  auto append = [&](xiiInt64 start, xiiInt64 end, xiiInt64 perpendicularStart, xiiInt64 perpendicularEnd, const xiiVec3& tangent, const xiiVec3& perpendicular) {
    for (xiiInt64 i = start; i <= end; ++i)
    {
      const xiiColor color    = i == 0 ? centerColor : (i % 10 == 0 ? majorColor : minorColor);
      const xiiVec3  position = center + tangent * static_cast<float>(i * spacing);
      m_Lines.PushBack(xiiDebugRendererLine(position + perpendicular * static_cast<float>(perpendicularStart * spacing), position + perpendicular * static_cast<float>(perpendicularEnd * spacing), color));
    }
  };
  m_Lines.Reserve(static_cast<xiiUInt32>(end1 - start1 + end2 - start2 + 2));
  append(start1, end1, start2, end2, tangent1, tangent2);
  append(start2, end2, start1, end1, tangent2, tangent1);
  for (const auto& line : m_Lines)
  {
    if (!line.m_vStart.IsValid() || !line.m_vEnd.IsValid())
    {
      m_Lines.Clear();
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}
