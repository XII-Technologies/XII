#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiCapsuleVisualizerAdapter::xiiCapsuleVisualizerAdapter()  = default;
xiiCapsuleVisualizerAdapter::~xiiCapsuleVisualizerAdapter() = default;

void xiiCapsuleVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");
  XII_MSVC_ANALYSIS_ASSUME(pAssetDocument != nullptr);

  const xiiCapsuleVisualizerAttribute* pAttr = static_cast<const xiiCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::CylinderZ, pAttr->m_Color, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);
  m_hSphereTop.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);
  m_hSphereBottom.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hCylinder);
  pAssetDocument->AddSyncObject(&m_hSphereTop);
  pAssetDocument->AddSyncObject(&m_hSphereBottom);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);
}

void xiiCapsuleVisualizerAdapter::Update()
{
  const xiiCapsuleVisualizerAttribute* pAttr           = static_cast<const xiiCapsuleVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*               pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;
  m_Anchor  = pAttr->m_Anchor;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    XII_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to xiiCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to xiiCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiCapsuleVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiCapsuleVisualizerAttribute 'color'");
    m_hSphereTop.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
    m_hSphereBottom.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
    m_hCylinder.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
  }
}

void xiiCapsuleVisualizerAdapter::UpdateGizmoTransform()
{
  xiiVec3 vOffset = xiiVec3::MakeZero();

  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosX))
    vOffset.x -= m_fRadius;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegX))
    vOffset.x += m_fRadius;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosY))
    vOffset.y -= m_fRadius;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegY))
    vOffset.y += m_fRadius;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosZ))
    vOffset.z -= m_fRadius + 0.5f * m_fHeight;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegZ))
    vOffset.z += m_fRadius + 0.5f * m_fHeight;

  xiiTransform tSphereTop;
  tSphereTop.SetIdentity();
  tSphereTop.m_vScale      = xiiVec3(m_fRadius);
  tSphereTop.m_vPosition.z = m_fHeight * 0.5f;
  tSphereTop.m_vPosition += vOffset;

  xiiTransform tSphereBottom;
  tSphereBottom.SetIdentity();
  tSphereBottom.m_vScale      = xiiVec3(m_fRadius, -m_fRadius, -m_fRadius);
  tSphereBottom.m_vPosition.z = -m_fHeight * 0.5f;
  tSphereBottom.m_vPosition += vOffset;

  xiiTransform tCylinder;
  tCylinder.SetIdentity();
  tCylinder.m_vScale = xiiVec3(m_fRadius, m_fRadius, m_fHeight);
  tCylinder.m_vPosition += vOffset;

  m_hSphereTop.SetTransformation(GetObjectTransform() * tSphereTop);
  m_hSphereBottom.SetTransformation(GetObjectTransform() * tSphereBottom);
  m_hCylinder.SetTransformation(GetObjectTransform() * tCylinder);
}
