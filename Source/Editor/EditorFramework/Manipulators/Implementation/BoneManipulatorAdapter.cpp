/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Animation/SkeletonPoseAuthoring.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiString xiiBoneManipulatorAdapter::s_sLastSelectedBone;

xiiBoneManipulatorAdapter::xiiBoneManipulatorAdapter()
{
  if (auto* curator = xiiAssetCurator::GetSingleton())
    curator->m_Events.AddEventHandler(xiiMakeDelegate(&xiiBoneManipulatorAdapter::AssetCuratorEventHandler, this), m_AssetEventsUnsubscriber);
}
xiiBoneManipulatorAdapter::~xiiBoneManipulatorAdapter() = default;

void xiiBoneManipulatorAdapter::Finalize()
{
  if (RetrieveBones().Failed())
    return;
  ConfigureGizmos();
}

void xiiBoneManipulatorAdapter::Update()
{
  m_bSkeletonMetadataDirty           = false;
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

void xiiBoneManipulatorAdapter::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  // Curator events arrive during its main-thread tick. Defer rebuilding window-owned gizmos until redraw.
  if (e.m_Type == xiiAssetCuratorEvent::Type::AssetListReset || ((e.m_Type == xiiAssetCuratorEvent::Type::AssetAdded || e.m_Type == xiiAssetCuratorEvent::Type::AssetRemoved || e.m_Type == xiiAssetCuratorEvent::Type::AssetUpdated || e.m_Type == xiiAssetCuratorEvent::Type::AssetMoved) && (!m_SkeletonAssetGuid.IsValid() || e.m_AssetGuid == m_SkeletonAssetGuid)))
    m_bSkeletonMetadataDirty = true;
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
      xiiMat4 inverseOwner = GetObjectTransform().GetAsMat4();
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
  m_SkeletonAssetGuid = {};
  m_Bones.Clear();
  m_Keys.Clear();
  m_ParentTransforms.Clear();
  auto*       accessor         = GetObjectAccessor();
  const auto* attribute        = static_cast<const xiiBoneManipulatorAttribute*>(m_pManipulatorAttr);
  const auto* property         = GetProperty(attribute->GetTransformProperty());
  const auto* skeletonProperty = GetProperty(attribute->m_sProperty2);
  if (property == nullptr || skeletonProperty == nullptr || property->GetCategory() != xiiPropertyCategory::Map || property->GetSpecificType() != xiiGetStaticRTTI<xiiTransform>())
    return XII_FAILURE;

  xiiVariant skeleton;
  XII_SUCCEED_OR_RETURN(accessor->GetValue(m_pObject, skeletonProperty, skeleton).GetResult());
  if (!skeleton.IsA<xiiString>() || xiiAssetCurator::GetSingleton() == nullptr)
    return XII_FAILURE;

  xiiDynamicArray<xiiExposedBone> metadata;
  {
    const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(skeleton.Get<xiiString>());
    if (!asset)
      return XII_FAILURE;
    m_SkeletonAssetGuid    = asset->m_Data.m_Guid;
    const auto* parameters = asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>();
    if (parameters == nullptr)
      return XII_FAILURE;
    for (const auto* parameter : parameters->m_Parameters)
    {
      if (parameter == nullptr || parameter->m_DefaultValue.GetReflectedType() != xiiGetStaticRTTI<xiiExposedBone>())
        return XII_FAILURE;
      metadata.PushBack(*static_cast<const xiiExposedBone*>(parameter->m_DefaultValue.GetData()));
    }
  }

  xiiVariantArray keys, values;
  XII_SUCCEED_OR_RETURN(accessor->GetKeys(m_pObject, property, keys).GetResult());
  XII_SUCCEED_OR_RETURN(accessor->GetValues(m_pObject, property, values).GetResult());
  if (keys.GetCount() != values.GetCount())
    return XII_FAILURE;
  xiiMap<xiiString, xiiTransform> overrides;
  for (xiiUInt32 i = 0; i < keys.GetCount(); ++i)
  {
    if (!keys[i].IsA<xiiString>() || !values[i].IsA<xiiTransform>())
      return XII_FAILURE;
    overrides.Insert(keys[i].Get<xiiString>(), values[i].Get<xiiTransform>());
  }
  XII_SUCCEED_OR_RETURN(xiiResolveSkeletonPoseForAuthoring(metadata, overrides, m_Bones));
  XII_SUCCEED_OR_RETURN(xiiExposedBone::ComputeParentTransforms(m_Bones, m_ParentTransforms));
  for (const auto& bone : m_Bones)
    m_Keys.PushBack(bone.m_sName);
  return XII_SUCCESS;
}

void xiiBoneManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_bSkeletonMetadataDirty)
  {
    m_bSkeletonMetadataDirty = false;
    Update();
    return;
  }
  if (m_ParentTransforms.GetCount() != m_Gizmos.GetCount() || m_Bones.GetCount() != m_Gizmos.GetCount())
    return;

  const xiiMat4 ownerTransform   = GetObjectTransform().GetAsMat4();
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
  if (uiBone >= m_Bones.GetCount())
    return;
  // Rotation handles must preserve the authored translation and non-uniform scale.
  auto       transform  = m_Bones[uiBone].m_Transform;
  const auto name       = m_Bones[uiBone].m_sName;
  transform.m_qRotation = value.m_qRotation;
  xiiMap<xiiString, xiiTransform> transforms;
  transforms.Insert(name, transform);
  const auto result = xiiApplySkeletonPoseForAuthoring(*GetObjectAccessor(), m_pObject, transforms);
  if (result.Failed())
    xiiLog::Error("Could not edit bone '{}': {}", name, result.GetMessageString());
}
