#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiBoxManipulatorAdapter::xiiBoxManipulatorAdapter()  = default;
xiiBoxManipulatorAdapter::~xiiBoxManipulatorAdapter() = default;

void xiiBoxManipulatorAdapter::QueryGridSettings(xiiGridSettingsMsgToEngine& outGridSettings)
{
  outGridSettings.m_vGridCenter = m_Gizmo.GetTransformation().m_vPosition;

  // if density != 0, it is enabled at least in ortho mode
  outGridSettings.m_fGridDensity = xiiSnapProvider::GetTranslationSnapValue();

  // to be active in perspective mode, tangents have to be non-zero
  outGridSettings.m_vGridTangent1.SetZero();
  outGridSettings.m_vGridTangent2.SetZero();
}

void xiiBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiBoxManipulatorAdapter::GizmoEventHandler, this));
}

void xiiBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  xiiObjectAccessorBase*            pObjectAccessor = GetObjectAccessor();
  const xiiBoxManipulatorAttribute* pAttr           = static_cast<const xiiBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    xiiVec3 vSize = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
    vSize *= pAttr->m_fSizeScale;
    vSize *= 0.5f;

    m_vOldSize = vSize;

    m_Gizmo.SetSize(vSize, vSize, false);
  }

  m_vPositionOffset.SetZero();

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    m_vPositionOffset = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetOffsetProperty()));
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<xiiQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  UpdateGizmoTransform();
}

void xiiBoxManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
      BeginTemporaryInteraction();
      break;

    case xiiGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case xiiGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case xiiGizmoEvent::Type::Interaction:
    {
      const xiiBoxManipulatorAttribute* pAttr = static_cast<const xiiBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const char*   szSizeProperty = pAttr->GetSizeProperty();
      const xiiVec3 vNewSizeNeg    = m_Gizmo.GetNegSize();
      const xiiVec3 vNewSizePos    = m_Gizmo.GetPosSize();
      const xiiVec3 vNewSize       = (vNewSizeNeg + vNewSizePos) / pAttr->m_fSizeScale;

      xiiVariant oldSize;

      xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

      pObjectAccessor->GetValue(m_pObject, GetProperty(szSizeProperty), oldSize);

      const xiiVec3 vOldSize = oldSize.ConvertTo<xiiVec3>();

      xiiVariant newValue = vNewSize;

      pObjectAccessor->StartTransaction("Change Properties");

      if (!xiiStringUtils::IsNullOrEmpty(szSizeProperty))
      {
        pObjectAccessor->SetValue(m_pObject, GetProperty(szSizeProperty), newValue);
      }

      if (pAttr->m_bRecenterParent)
      {
        const xiiDocumentObject* pParent = m_pObject->GetParent();

        if (const xiiGameObjectDocument* pGameDoc = xiiDynamicCast<const xiiGameObjectDocument*>(pParent->GetDocumentObjectManager()->GetDocument()))
        {
          xiiTransform tParent = pGameDoc->GetGlobalTransform(pParent);

          xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

          if (m_vOldSize.x != vNewSizeNeg.x)
            tParent.m_vPosition -= tParent.m_qRotation * xiiVec3((vNewSizeNeg.x - m_vOldSize.x) * 0.5f, 0, 0);
          if (m_vOldSize.x != vNewSizePos.x)
            tParent.m_vPosition += tParent.m_qRotation * xiiVec3((vNewSizePos.x - m_vOldSize.x) * 0.5f, 0, 0);

          if (m_vOldSize.y != vNewSizeNeg.y)
            tParent.m_vPosition -= tParent.m_qRotation * xiiVec3(0, (vNewSizeNeg.y - m_vOldSize.y) * 0.5f, 0);
          if (m_vOldSize.y != vNewSizePos.y)
            tParent.m_vPosition += tParent.m_qRotation * xiiVec3(0, (vNewSizePos.y - m_vOldSize.y) * 0.5f, 0);

          if (m_vOldSize.z != vNewSizeNeg.z)
            tParent.m_vPosition -= tParent.m_qRotation * xiiVec3(0, 0, (vNewSizeNeg.z - m_vOldSize.z) * 0.5f);
          if (m_vOldSize.z != vNewSizePos.z)
            tParent.m_vPosition += tParent.m_qRotation * xiiVec3(0, 0, (vNewSizePos.z - m_vOldSize.z) * 0.5f);

          pGameDoc->SetGlobalTransform(pParent, tParent, TransformationChanges::Translation);
        }
      }

      pObjectAccessor->FinishTransaction();
    }

    break;
  }
}

void xiiBoxManipulatorAdapter::UpdateGizmoTransform()
{
  xiiTransform t;
  t.m_vScale.Set(1);
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_Gizmo.SetTransformation(GetObjectTransform() * t);
}
