#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiSpotLightVisualizerAdapter::xiiSpotLightVisualizerAdapter() {}

xiiSpotLightVisualizerAdapter::~xiiSpotLightVisualizerAdapter() {}

void xiiSpotLightVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Cone, xiiColor::White, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiSpotLightVisualizerAdapter::Update()
{
  const xiiSpotLightVisualizerAttribute* pAttr           = static_cast<const xiiSpotLightVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*                 pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiAngle>(), "Invalid property bound to xiiSpotLightVisualizerAttribute 'angle'");
    m_fAngleScale = xiiMath::Tan(value.ConvertTo<xiiAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiSpotLightVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>());
  }

  m_fScale = 1.0f;
  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    xiiVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range);
    XII_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to xiiPointLightVisualizerAttribute 'radius'");

    xiiVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity);
    XII_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to xiiPointLightVisualizerAttribute 'intensity'");

    m_fScale = xiiLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  m_hGizmo.SetVisible(m_fAngleScale != 0.0f && m_fScale != 0.0f);
}

void xiiSpotLightVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t = GetObjectTransform();
  t.m_vScale     = t.m_vScale.CompMul(xiiVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fScale);
  m_hGizmo.SetTransformation(t);
}
