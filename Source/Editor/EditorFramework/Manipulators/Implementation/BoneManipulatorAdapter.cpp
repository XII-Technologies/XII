/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiString xiiBoneManipulatorAdapter::s_sLastSelectedBone;

xiiBoneManipulatorAdapter::xiiBoneManipulatorAdapter()  = default;
xiiBoneManipulatorAdapter::~xiiBoneManipulatorAdapter() = default;

void xiiBoneManipulatorAdapter::Finalize()
{
  if (RetrieveBones().Failed())
    return;
  ConfigureGizmos();
}

void xiiBoneManipulatorAdapter::Update()
{
  const xiiVariantArray previousKeys = m_Keys;
  if (RetrieveBones().Failed())
  {
    m_Gizmos.Clear();
    return;
  }

  if (previousKeys != m_Keys || m_Gizmos.GetCount() != m_Bones.GetCount())
  {
    m_Gizmos.Clear();
    ConfigureGizmos();
    return;
  }
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

      xiiMat4 mGizmo       = globalGizmo.GetAsMat4();
      xiiMat4 inverseOwner = GetObjectTransform().GetAsMat4() * m_RootTransform.GetAsMat4();
      if (inverseOwner.Invert().Failed())
        return;
      mGizmo = inverseOwner * mGizmo;

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
      UpdateGizmoTransform();
    }
    break;
    default:
      break;
  }
}

xiiResult xiiBoneManipulatorAdapter::RetrieveBones()
{
  m_Bones.Clear();
  m_Keys.Clear();
  m_ParentTransforms.Clear();
  m_RootTransform.SetIdentity();
  xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const xiiBoneManipulatorAttribute* pAttr = static_cast<const xiiBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return XII_FAILURE;

  xiiVariantArray values;

  // Exposed parameters are only stored as diffs in the component. Thus, requesting the exposed parameters only returns those that have been modified. To get all, you need to use the xiiExposedParameterCommandAccessor which gives you all exposed parameters from the source asset.
  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (pProperty == nullptr)
    return XII_FAILURE;
  if (const xiiExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<xiiExposedParametersAttribute>())
  {
    const xiiAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    if (pParameterSourceProp == nullptr)
      return XII_FAILURE;

    xiiExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    XII_SUCCEED_OR_RETURN(proxy.GetValues(m_pObject, pProperty, values).GetResult());
    XII_SUCCEED_OR_RETURN(proxy.GetKeys(m_pObject, pProperty, m_Keys).GetResult());
  }
  else
  {
    XII_SUCCEED_OR_RETURN(pObjectAccessor->GetKeys(m_pObject, pProperty, m_Keys).GetResult());
    XII_SUCCEED_OR_RETURN(pObjectAccessor->GetValues(m_pObject, pProperty, values).GetResult());
  }

  if (values.GetCount() != m_Keys.GetCount())
    return XII_FAILURE;
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
      m_Bones.Clear();
      return XII_FAILURE;
    }
  }

  if (xiiExposedBone::ComputeParentTransforms(m_Bones, m_ParentTransforms).Failed())
  {
    m_Bones.Clear();
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiBoneManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_ParentTransforms.GetCount() != m_Gizmos.GetCount() || m_Bones.GetCount() != m_Gizmos.GetCount())
    return;

  const xiiMat4 ownerTransform   = GetObjectTransform().GetAsMat4() * m_RootTransform.GetAsMat4();
  xiiMat4       inverseOwner     = ownerTransform;
  const bool    bOwnerInvertible = inverseOwner.Invert().Succeeded();

  for (xiiUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset        = m_ParentTransforms[i];
    gizmo.m_InverseOffset = gizmo.m_Offset;
    if (!bOwnerInvertible || gizmo.m_InverseOffset.Invert().Failed())
    {
      gizmo.m_RotateGizmo.SetVisible(false);
      gizmo.m_ClickGizmo.SetVisible(false);
      continue;
    }

    xiiMat4 mGizmo       = ownerTransform * gizmo.m_Offset * m_Bones[i].m_Transform.GetAsMat4();
    xiiMat4 inverseGizmo = mGizmo;
    if (inverseGizmo.Invert().Failed())
    {
      gizmo.m_RotateGizmo.SetVisible(false);
      gizmo.m_ClickGizmo.SetVisible(false);
      continue;
    }

    const bool bSelected = m_Bones[i].m_sName == s_sLastSelectedBone;
    gizmo.m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && bSelected);
    gizmo.m_ClickGizmo.SetVisible(m_bManipulatorIsVisible && !bSelected);

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
  if (pEngineWindow == nullptr)
    return;

  m_Gizmos.SetCount(m_Bones.GetCount());

  for (xiiUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = m_ParentTransforms[i];

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

  // Rotation handles must preserve the authored translation and non-uniform scale.
  xiiExposedBone bone          = *pBone;
  bone.m_Transform.m_qRotation = value.m_qRotation;

  xiiVariant var;
  var.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());

  xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const xiiBoneManipulatorAttribute* pAttr = static_cast<const xiiBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (pProperty == nullptr)
    return;
  const xiiExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<xiiExposedParametersAttribute>();

  xiiStatus result = XII_FAILURE;
  if (pAttrib != nullptr)
  {
    const xiiAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    if (pParameterSourceProp == nullptr)
      return;
    xiiExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    result = proxy.SetValue(m_pObject, pProperty, var, m_Keys[uiBone]);
  }
  else
  {
    result = pObjectAccessor->SetValue(m_pObject, pProperty, var, m_Keys[uiBone]);
  }

  if (result.Failed())
    xiiLog::Error("Could not edit bone '{}': {}", bone.m_sName, result.GetMessageString());
}
