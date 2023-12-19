#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiDirectionVisualizerAdapter::xiiDirectionVisualizerAdapter() = default;

xiiDirectionVisualizerAdapter::~xiiDirectionVisualizerAdapter() = default;

void xiiDirectionVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  const xiiDirectionVisualizerAttribute* pAttr = static_cast<const xiiDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Arrow, pAttr->m_Color, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiDirectionVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  const xiiDirectionVisualizerAttribute* pAttr = static_cast<const xiiDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiColor>(), "Invalid property bound to xiiDirectionVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<xiiColor>() * pAttr->m_Color);
  }
}

void xiiDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  const xiiDirectionVisualizerAttribute* pAttr  = static_cast<const xiiDirectionVisualizerAttribute*>(m_pVisualizerAttr);
  float                                  fScale = pAttr->m_fScale;

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to xiiDirectionVisualizerAttribute 'length'");
    fScale *= value.ConvertTo<float>();
  }

  xiiBasisAxis::Enum axis = pAttr->m_Axis;

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    xiiVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).AssertSuccess();

    XII_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<xiiInt32>(), "Invalid property bound to xiiDirectionVisualizerAttribute 'length'");

    axis = static_cast<xiiBasisAxis::Enum>(value.ConvertTo<xiiInt32>());
  }

  const xiiQuat axisRotation = xiiBasisAxis::GetBasisRotation_PosX(axis);

  xiiTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale    = xiiVec3(fScale);
  t.m_vPosition = axisRotation * xiiVec3(fScale * 0.5f, 0, 0);

  xiiTransform tObject = GetObjectTransform();
  tObject.m_vScale.Set(1.0f);

  m_hGizmo.SetTransformation(tObject * t);
}
