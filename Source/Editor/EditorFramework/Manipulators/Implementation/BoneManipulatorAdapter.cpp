#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiString xiiBoneManipulatorAdapter::s_sLastSelectedBone;

xiiBoneManipulatorAdapter::xiiBoneManipulatorAdapter()  = default;
xiiBoneManipulatorAdapter::~xiiBoneManipulatorAdapter() = default;

void xiiBoneManipulatorAdapter::Finalize()
{
  RetrieveBones();
  ConfigureGizmos();
  MigrateSelection();
}

void xiiBoneManipulatorAdapter::MigrateSelection()
{
  for (xiiUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (m_Bones[i].m_sName == s_sLastSelectedBone)
    {
      m_Gizmos[i].m_RotateGizmo.SetVisible(true);
      m_Gizmos[i].m_ClickGizmo.SetVisible(false);
      return;
    }
  }

  // keep the last selection, even if it can't be migrated, until something else gets selected
}

void xiiBoneManipulatorAdapter::Update()
{
  RetrieveBones();
  UpdateGizmoTransform();
}

void xiiBoneManipulatorAdapter::RotateGizmoEventHandler(const xiiGizmoEvent& e)
{
  xiiUInt32 uiGizmo = xiiInvalidIndex;

  for (xiiUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_RotateGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      break;
    }
  }

  XII_ASSERT_DEBUG(uiGizmo != xiiInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == xiiInvalidIndex)
    return;

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
      xiiTransform globalGizmo = static_cast<const xiiGizmo*>(e.m_pGizmo)->GetTransformation();
      globalGizmo.m_vScale.Set(1);

      xiiMat4 mGizmo = globalGizmo.GetAsMat4();
      mGizmo         = GetObjectTransform().GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_RootTransform.GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_Gizmos[uiGizmo].m_InverseOffset * mGizmo;

      xiiQuat rotOnly;
      rotOnly.ReconstructFromMat4(mGizmo);

      SetTransform(uiGizmo, xiiTransform(mGizmo.GetTranslationVector(), rotOnly));
    }
    break;
  }
}

void xiiBoneManipulatorAdapter::ClickGizmoEventHandler(const xiiGizmoEvent& e)
{
  xiiUInt32 uiGizmo = xiiInvalidIndex;
  s_sLastSelectedBone.Clear();

  for (xiiUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_ClickGizmo == e.m_pGizmo)
    {
      uiGizmo             = gIdx;
      s_sLastSelectedBone = m_Bones[gIdx].m_sName;
      break;
    }
  }

  XII_ASSERT_DEBUG(uiGizmo != xiiInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == xiiInvalidIndex)
    return;

  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::Interaction:
    {
      for (xiiUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
      {
        m_Gizmos[i].m_RotateGizmo.SetVisible(false);
        m_Gizmos[i].m_ClickGizmo.SetVisible(true);
      }

      m_Gizmos[uiGizmo].m_RotateGizmo.SetVisible(true);
      m_Gizmos[uiGizmo].m_ClickGizmo.SetVisible(false);
    }
    break;
    default:
      break;
  }
}

void xiiBoneManipulatorAdapter::RetrieveBones()
{
  xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const xiiBoneManipulatorAttribute* pAttr = static_cast<const xiiBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  xiiVariantArray values;

  // Exposed parameters are only stored as diffs in the component. Thus, requesting the exposed parameters only returns those that have been modified. To get all, you need to use the xiiExposedParameterCommandAccessor which gives you all exposed parameters from the source asset.
  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (const xiiExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<xiiExposedParametersAttribute>())
  {
    const xiiAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    XII_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

    xiiExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    proxy.GetValues(m_pObject, pProperty, values).AssertSuccess();
    proxy.GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
  }
  else
  {
    pObjectAccessor->GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
    pObjectAccessor->GetValues(m_pObject, pProperty, values).AssertSuccess();
  }

  m_RootTransform.SetIdentity();

  m_Bones.Clear();
  m_Bones.SetCount(values.GetCount());

  for (xiiUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (values[i].GetReflectedType() == xiiGetStaticRTTI<xiiExposedBone>())
    {
      const xiiExposedBone* pBone = reinterpret_cast<const xiiExposedBone*>(values[i].GetData());

      if (pBone->m_sName == "<root-transform>")
      {
        m_RootTransform = pBone->m_Transform;
      }

      m_Bones[i] = *pBone;
    }
    else
    {
      //XII_REPORT_FAILURE("Property is not a xiiExposedBone");
      m_Bones.Clear();
      return;
    }
  }
}

void xiiBoneManipulatorAdapter::UpdateGizmoTransform()
{
  const xiiMat4 ownerTransform = GetObjectTransform().GetAsMat4();

  for (xiiUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset        = ComputeParentTransform(i);
    gizmo.m_InverseOffset = gizmo.m_Offset.GetInverse();

    xiiMat4 mGizmo = ownerTransform * m_RootTransform.GetAsMat4() * gizmo.m_Offset * m_Bones[i].m_Transform.GetAsMat4();

    xiiQuat rotOnly;
    rotOnly.ReconstructFromMat4(mGizmo);

    xiiTransform tGizmo;
    tGizmo.m_vPosition = mGizmo.GetTranslationVector();
    tGizmo.m_qRotation = rotOnly;

    tGizmo.m_vScale.Set(0.5f);
    gizmo.m_RotateGizmo.SetTransformation(tGizmo);

    tGizmo.m_vScale.Set(0.02f);
    gizmo.m_ClickGizmo.SetTransformation(tGizmo);
  }
}

void xiiBoneManipulatorAdapter::ConfigureGizmos()
{
  auto*                      pDoc          = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto*                      pWindow       = xiiQtDocumentWindow::FindWindowByDocument(pDoc);
  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmos.SetCount(m_Bones.GetCount());

  for (xiiUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);

    auto& rot = gizmo.m_RotateGizmo;
    rot.SetOwner(pEngineWindow, nullptr);
    rot.SetVisible(false);
    rot.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiBoneManipulatorAdapter::RotateGizmoEventHandler, this));

    auto& click = gizmo.m_ClickGizmo;
    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(xiiColor::Thistle);
    click.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiBoneManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

void xiiBoneManipulatorAdapter::SetTransform(xiiUInt32 uiBone, const xiiTransform& value)
{
  xiiExposedBone* pBone = &m_Bones[uiBone];

  pBone->m_Transform.m_qRotation = value.m_qRotation;
  pBone->m_Transform             = value;

  xiiExposedBone bone;
  bone.m_sName     = pBone->m_sName;
  bone.m_sParent   = pBone->m_sParent;
  bone.m_Transform = value;

  xiiVariant var;
  var.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());

  xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const xiiBoneManipulatorAttribute* pAttr = static_cast<const xiiBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  auto                                 pProperty = GetProperty(pAttr->GetTransformProperty());
  const xiiExposedParametersAttribute* pAttrib   = pProperty->GetAttributeByType<xiiExposedParametersAttribute>();

  const xiiAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
  XII_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

  xiiExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);

  // for some reason the first command in xiiExposedParameterCommandAccessor returns failure 'the property X does not exist' and the insert
  // command than fails with 'the property X already exists' ???

  proxy.SetValue(m_pObject, pProperty, var, m_Keys[uiBone]).AssertSuccess();
}

xiiMat4 xiiBoneManipulatorAdapter::ComputeFullTransform(xiiUInt32 uiBone) const
{
  const xiiMat4 tParent = ComputeParentTransform(uiBone);

  return tParent * m_Bones[uiBone].m_Transform.GetAsMat4();
}

xiiMat4 xiiBoneManipulatorAdapter::ComputeParentTransform(xiiUInt32 uiBone) const
{
  const xiiString& parent = m_Bones[uiBone].m_sParent;

  if (!parent.IsEmpty())
  {
    for (xiiUInt32 b = 0; b < m_Bones.GetCount(); ++b)
    {
      if (m_Bones[b].m_sName == parent)
      {
        return ComputeFullTransform(b);
      }
    }
  }

  return xiiMat4::IdentityMatrix();
}
