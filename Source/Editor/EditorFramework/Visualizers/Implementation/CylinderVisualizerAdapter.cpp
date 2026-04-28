/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiCylinderVisualizerAdapter::xiiCylinderVisualizerAdapter() = default;

xiiCylinderVisualizerAdapter::~xiiCylinderVisualizerAdapter() = default;

void xiiCylinderVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiCylinderVisualizerAttribute* pAttr = static_cast<const xiiCylinderVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::CylinderZ, pAttr->m_Color, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hCylinder);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
}

void xiiCylinderVisualizerAdapter::Update()
{
  const xiiCylinderVisualizerAttribute* pAttr           = static_cast<const xiiCylinderVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*                pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    XII_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to xiiCylinderVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to xiiCylinderVisualizerAttribute 'radius'",
                     pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCylinderVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiCylinderVisualizerAttribute 'color'");
    m_hCylinder.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiVec3>(), "Invalid property bound to xiiCylinderVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<xiiVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<xiiVec3>());
  }

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiInt32>(), "Invalid property bound to xiiCylinderVisualizerAttribute 'axis'");

    m_Axis = static_cast<xiiBasisAxis::Enum>(value.ConvertTo<xiiInt32>());
  }
  else
  {
    m_Axis = pAttr->m_Axis;
  }

  m_Anchor = pAttr->m_Anchor;
}

void xiiCylinderVisualizerAdapter::UpdateGizmoTransform()
{
  const xiiQuat axisRotation = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveZ, m_Axis);

  xiiTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale    = xiiVec3(m_fRadius, m_fRadius, m_fHeight);
  t.m_vPosition = m_vPositionOffset;

  xiiVec3 vOffset = xiiVec3::MakeZero();

  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  // xiiTransform doesn't (can't) combine rotations with scales
  // however, here we know that the axisRotation is just an axis remapping, so we can combine them
  xiiTransform parentTransform = GetObjectTransform();
  xiiTransform newTrans        = parentTransform * t;
  newTrans.m_vScale            = (axisRotation * parentTransform.m_vScale).CompMul(t.m_vScale);

  m_hCylinder.SetTransformation(newTrans);
}
