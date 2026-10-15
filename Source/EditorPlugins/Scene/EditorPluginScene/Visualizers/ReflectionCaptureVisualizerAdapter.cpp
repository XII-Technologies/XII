/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/ReflectionCaptureVisualizerAdapter.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void xiiReflectionCaptureVisualizerAdapter::Finalize()
{
  const auto* pDocument = xiiDynamicCast<const xiiAssetDocument*>(m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument());
  XII_ASSERT_DEV(pDocument != nullptr, "Visualizers require an asset document.");
  if (pDocument == nullptr)
    return;

  const auto flags      = xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer;
  const auto outerColor = xiiColorScheme::LightUI(xiiColorScheme::Yellow);
  const auto innerColor = xiiColorScheme::LightUI(xiiColorScheme::Orange);
  m_hBox.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, outerColor, flags);
  m_hInnerBox.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, innerColor, flags);
  m_hSphere.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, outerColor, flags);
  m_hInnerSphere.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, innerColor, flags);
  for (auto* handle : {&m_hBox, &m_hInnerBox, &m_hSphere, &m_hInnerSphere})
    pDocument->AddSyncObject(handle);
}

void xiiReflectionCaptureVisualizerAdapter::Update()
{
  for (auto* handle : {&m_hBox, &m_hInnerBox, &m_hSphere, &m_hInnerSphere})
    handle->SetVisible(false);
  const auto* attr     = static_cast<const xiiReflectionCaptureVisualizerAttribute*>(m_pVisualizerAttr);
  auto*       accessor = GetObjectAccessor();
  xiiVariant  shape, extents, radius, blend;
  if (accessor->GetValueByName(m_pObject, attr->GetShapeProperty(), shape).Failed() || !shape.CanConvertTo<xiiInt64>() || accessor->GetValueByName(m_pObject, attr->GetHalfExtentsProperty(), extents).Failed() || !extents.IsA<xiiVec3>() || accessor->GetValueByName(m_pObject, attr->GetRadiusProperty(), radius).Failed() || !radius.CanConvertTo<float>() || accessor->GetValueByName(m_pObject, attr->GetBlendDistanceProperty(), blend).Failed() || !blend.CanConvertTo<float>())
    return;

  const xiiVec3 halfExtents   = extents.Get<xiiVec3>();
  const float   sphereRadius  = radius.ConvertTo<float>();
  const float   blendDistance = blend.ConvertTo<float>();
  if (!xiiMath::IsFinite(blendDistance))
    return;

  // Match native component bounds and the blend threshold in ReflectionProbeSampling.h.
  const float clampedBlend   = xiiMath::Max(blendDistance, 0.0f);
  const auto  influenceShape = shape.ConvertTo<xiiInt64>();
  if (influenceShape == xiiReflectionProbeInfluenceShape::Box)
  {
    if (!halfExtents.IsValid())
      return;
    const auto clampedExtents = halfExtents.CompMax(xiiVec3(0.01f));
    m_vOuterScale             = clampedExtents * 2.0f;
    m_vInnerScale             = (clampedExtents - xiiVec3(clampedBlend)).CompMax(xiiVec3::MakeZero()) * 2.0f;
    m_hBox.SetVisible(m_bVisualizerIsVisible);
    m_hInnerBox.SetVisible(m_bVisualizerIsVisible && clampedBlend > 0.0001f && xiiMath::Min(clampedExtents.x, clampedExtents.y, clampedExtents.z) > clampedBlend);
  }
  else if (influenceShape == xiiReflectionProbeInfluenceShape::Sphere)
  {
    if (!xiiMath::IsFinite(sphereRadius))
      return;
    const float clampedRadius = xiiMath::Max(sphereRadius, 0.01f);
    m_vOuterScale.Set(clampedRadius);
    m_vInnerScale.Set(xiiMath::Max(clampedRadius - clampedBlend, 0.0f));
    m_hSphere.SetVisible(m_bVisualizerIsVisible);
    m_hInnerSphere.SetVisible(m_bVisualizerIsVisible && clampedBlend > 0.0001f && clampedRadius > clampedBlend);
  }
  UpdateGizmoTransform();
}

void xiiReflectionCaptureVisualizerAdapter::UpdateGizmoTransform()
{
  const auto objectTransform = GetObjectTransform();
  auto       outer           = xiiTransform::MakeIdentity();
  auto       inner           = xiiTransform::MakeIdentity();
  outer.m_vScale             = m_vOuterScale;
  inner.m_vScale             = m_vInnerScale;
  for (auto* handle : {&m_hBox, &m_hSphere})
    handle->SetTransformation(objectTransform * outer);
  for (auto* handle : {&m_hInnerBox, &m_hInnerSphere})
    handle->SetTransformation(objectTransform * inner);
}
