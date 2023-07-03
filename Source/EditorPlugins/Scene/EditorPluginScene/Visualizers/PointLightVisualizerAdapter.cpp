#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiPointLightVisualizerAdapter::xiiPointLightVisualizerAdapter() {}

xiiPointLightVisualizerAdapter::~xiiPointLightVisualizerAdapter() {}

void xiiPointLightVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, xiiColor::White, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiPointLightVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  xiiObjectAccessorBase*                  pObjectAccessor = GetObjectAccessor();
  const xiiPointLightVisualizerAttribute* pAttr           = static_cast<const xiiPointLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    xiiVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    XII_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to xiiPointLightVisualizerAttribute 'radius'");

    xiiVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    XII_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to xiiPointLightVisualizerAttribute 'intensity'");

    m_fScale = xiiLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiPointLightVisualizerAdapter 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>());
  }
}

void xiiPointLightVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t = GetObjectTransform();
  t.m_vScale *= m_fScale;

  m_hGizmo.SetTransformation(t);
}
