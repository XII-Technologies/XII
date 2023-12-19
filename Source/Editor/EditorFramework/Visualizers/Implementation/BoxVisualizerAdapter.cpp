#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiBoxVisualizerAdapter::xiiBoxVisualizerAdapter()  = default;
xiiBoxVisualizerAdapter::~xiiBoxVisualizerAdapter() = default;

void xiiBoxVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiBoxVisualizerAttribute* pAttr = static_cast<const xiiBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, pAttr->m_Color, xiiGizmoFlags::Visualizer | xiiGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiBoxVisualizerAdapter::Update()
{
  const xiiBoxVisualizerAttribute* pAttr           = static_cast<const xiiBoxVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*           pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(pAttr->m_fSizeScale);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    m_vScale *= pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiBoxVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiVec3>(), "Invalid property bound to xiiBoxVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<xiiVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<xiiVec3>());
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<xiiQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  m_Anchor = pAttr->m_Anchor;
}

void xiiBoxVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t;
  t.m_vScale    = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  xiiVec3 vOffset = xiiVec3::MakeZero();

  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(xiiVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
