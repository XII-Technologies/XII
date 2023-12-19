#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <GraphicsCore/Lights/BoxReflectionProbeComponent.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiBoxReflectionProbeVisualizerAdapter::xiiBoxReflectionProbeVisualizerAdapter()  = default;
xiiBoxReflectionProbeVisualizerAdapter::~xiiBoxReflectionProbeVisualizerAdapter() = default;

void xiiBoxReflectionProbeVisualizerAdapter::Finalize()
{
  auto*                   pDoc           = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const xiiAssetDocument* pAssetDocument = xiiDynamicCast<const xiiAssetDocument*>(pDoc);
  XII_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in xiiAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::LineBox, xiiColorScheme::LightUI(xiiColorScheme::Yellow), xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void xiiBoxReflectionProbeVisualizerAdapter::Update()
{
  const xiiBoxReflectionProbeVisualizerAttribute* pAttr           = static_cast<const xiiBoxReflectionProbeVisualizerAttribute*>(m_pVisualizerAttr);
  xiiObjectAccessorBase*                          pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(1.0f);
  xiiVec3 influenceScale;
  xiiVec3 influenceShift;

  if (!pAttr->GetExtentsProperty().IsEmpty())
  {
    m_vScale = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetExtentsProperty()));
  }

  if (!pAttr->GetInfluenceScaleProperty().IsEmpty())
  {
    influenceScale = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetInfluenceScaleProperty()));
  }

  if (!pAttr->GetInfluenceShiftProperty().IsEmpty())
  {
    influenceShift = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetInfluenceShiftProperty()));
  }

  m_vPositionOffset = m_vScale.CompMul(influenceShift.CompMul(xiiVec3(1.0f) - influenceScale)) * 0.5f;
  m_vScale *= influenceScale;

  m_qRotation.SetIdentity();
}

void xiiBoxReflectionProbeVisualizerAdapter::UpdateGizmoTransform()
{
  xiiTransform t;
  t.m_vScale    = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
