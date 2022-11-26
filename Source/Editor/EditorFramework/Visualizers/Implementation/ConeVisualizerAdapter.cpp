#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiConeVisualizerAdapter::xiiConeVisualizerAdapter() {}

xiiConeVisualizerAdapter::~xiiConeVisualizerAdapter() {}

void xiiConeVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiConeVisualizerAttribute* pAttr = static_cast<const xiiConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Cone, pAttr->m_Color, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiConeVisualizerAdapter::Update()
{
  const xiiConeVisualizerAttribute* pAttr           = static_cast<const xiiConeVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*            pObjectAccessor = GetObjectAccessor();

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiAngle>(), "Invalid property bound to xiiConeVisualizerAttribute 'angle'");
    m_fAngleScale = xiiMath::Tan(value.ConvertTo<xiiAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiConeVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>());
  }

  m_fFinalScale = pAttr->m_fScale;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiConeVisualizerAttribute 'radius'");
    m_fFinalScale *= value.ConvertTo<float>();
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale != 0.0f && m_fFinalScale != 0.0f);
}

void xiiConeVisualizerAdapter::UpdateGizmoTransform()
{
  const xiiConeVisualizerAttribute* pAttr = static_cast<const xiiConeVisualizerAttribute*>(m_pVisualizerAttr);

  const xiiQuat axisRotation = xiiBasisAxis::GetBasisRotation_PosX(pAttr->m_Axis);

  xiiTransform t = GetObjectTransform();
  t.m_vScale     = t.m_vScale.CompMul(xiiVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fFinalScale);
  t.m_qRotation  = axisRotation * t.m_qRotation;
  m_hGizmo.SetTransformation(t);
}
