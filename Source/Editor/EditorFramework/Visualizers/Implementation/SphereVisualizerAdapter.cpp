#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiSphereVisualizerAdapter::xiiSphereVisualizerAdapter()  = default;
xiiSphereVisualizerAdapter::~xiiSphereVisualizerAdapter() = default;

void xiiSphereVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiSphereVisualizerAttribute* pAttr = static_cast<const xiiSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, pAttr->m_Color, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiSphereVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  xiiObjectAccessorBase*              pObjectAccessor = GetObjectAccessor();
  const xiiSphereVisualizerAttribute* pAttr           = static_cast<const xiiSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiSphereVisualizerAttribute 'radius'");
    m_fScale = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiSphereVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value);

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiVec3>(), "Invalid property bound to xiiSphereVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<xiiVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<xiiVec3>());
  }

  m_Anchor = pAttr->m_Anchor;
}

void xiiSphereVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t;
  t.m_qRotation.SetIdentity();
  t.m_vScale.Set(m_fScale);
  t.m_vPosition = m_vPositionOffset;

  xiiVec3 vOffset = xiiVec3::ZeroVector();

  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
