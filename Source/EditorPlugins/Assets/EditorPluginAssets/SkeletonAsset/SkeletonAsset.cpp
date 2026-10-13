/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Declarations.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreviewMessages.h>
#include <ToolsFoundation/Animation/ExposedBone.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonAssetDocument, 11, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkeletonAssetDocument::xiiSkeletonAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiEditableSkeleton>(sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiSkeletonAssetDocument::~xiiSkeletonAssetDocument() = default;

void xiiSkeletonAssetDocument::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiEditableSkeleton>())
  {
    auto& props = *e.m_pPropertyStates;

    const xiiInt64 importTransform       = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<xiiInt64>();
    const bool     bCustomTransform      = importTransform == 127;
    props["RightDir"].m_Visibility       = bCustomTransform ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["UpDir"].m_Visibility          = bCustomTransform ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["FlipForwardDir"].m_Visibility = bCustomTransform ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiEditableSkeletonJoint>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool                       overrideSurface        = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool                       overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();
    const xiiSkeletonJointType::Enum jointType              = (xiiSkeletonJointType::Enum)e.m_pObject->GetTypeAccessor().GetValue("JointType").ConvertTo<xiiInt32>();

    const bool bHasStiffness = jointType == xiiSkeletonJointType::SwingTwist;
    const bool bHasSwing     = jointType == xiiSkeletonJointType::SwingTwist;
    const bool bHasTwist     = jointType == xiiSkeletonJointType::SwingTwist;

    props["CollisionLayer"].m_Visibility = overrideCollisionLayer ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["Surface"].m_Visibility        = overrideSurface ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;

    props["LocalRotation"].m_Visibility         = bHasStiffness ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["Stiffness"].m_Visibility             = bHasStiffness ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["SwingLimitY"].m_Visibility           = bHasSwing ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["SwingLimitZ"].m_Visibility           = bHasSwing ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["TwistLimitHalfAngle"].m_Visibility   = bHasTwist ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["TwistLimitCenterAngle"].m_Visibility = bHasTwist ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;

    return;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiEditableSkeletonBoneShape>())
  {
    auto& props = *e.m_pPropertyStates;

    const xiiSkeletonJointGeometryType::Enum geomType = (xiiSkeletonJointGeometryType::Enum)e.m_pObject->GetTypeAccessor().GetValue("Geometry").ConvertTo<xiiInt32>();

    props["Offset"].m_Visibility    = xiiPropertyUiState::Invisible;
    props["Rotation"].m_Visibility  = xiiPropertyUiState::Invisible;
    props["Length"].m_Visibility    = xiiPropertyUiState::Invisible;
    props["Width"].m_Visibility     = xiiPropertyUiState::Invisible;
    props["Thickness"].m_Visibility = xiiPropertyUiState::Invisible;

    if (geomType == xiiSkeletonJointGeometryType::None)
      return;

    props["Length"].m_sNewLabelText    = "Length";
    props["Width"].m_sNewLabelText     = "Width";
    props["Thickness"].m_sNewLabelText = "Thickness";

    props["Offset"].m_Visibility   = xiiPropertyUiState::Default;
    props["Rotation"].m_Visibility = xiiPropertyUiState::Default;

    if (geomType == xiiSkeletonJointGeometryType::Box)
    {
      props["Length"].m_Visibility    = xiiPropertyUiState::Default;
      props["Width"].m_Visibility     = xiiPropertyUiState::Default;
      props["Thickness"].m_Visibility = xiiPropertyUiState::Default;
    }
    else if (geomType == xiiSkeletonJointGeometryType::Sphere)
    {
      props["Thickness"].m_Visibility    = xiiPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }
    else if (geomType == xiiSkeletonJointGeometryType::Capsule)
    {
      props["Length"].m_Visibility = xiiPropertyUiState::Default;

      props["Thickness"].m_Visibility    = xiiPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }

    return;
  }
}

xiiStatus xiiSkeletonAssetDocument::WriteResource(xiiStreamWriter& inout_stream, const xiiEditableSkeleton& skeleton) const
{
  xiiSkeletonResourceDescriptor desc;
  if (skeleton.FillResourceDescriptor(desc).Failed())
    return xiiStatus("Skeleton hierarchy, joint names or import/rest transforms are invalid.");

  XII_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return XII_SUCCESS;
}

xiiStatus xiiSkeletonAssetDocument::SendPreviewConfiguration() const
{
  xiiEditorEngineSkeletonPreviewMsg message;
  if (message.SetSkeleton(*GetProperties()).Failed())
    return xiiStatus("Cannot preview an invalid skeleton hierarchy or rest pose.");
  if (!SendMessageToEngine(&message))
    return xiiStatus("Could not send skeleton preview configuration to the engine process.");
  return XII_SUCCESS;
}

void xiiSkeletonAssetDocument::SetRenderBones(bool bEnable)
{
  if (m_bRenderBones == bEnable)
    return;

  m_bRenderBones = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::SetRenderColliders(bool bEnable)
{
  if (m_bRenderColliders == bEnable)
    return;

  m_bRenderColliders = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::SetRenderJoints(bool bEnable)
{
  if (m_bRenderJoints == bEnable)
    return;

  m_bRenderJoints = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::SetRenderSwingLimits(bool bEnable)
{
  if (m_bRenderSwingLimits == bEnable)
    return;

  m_bRenderSwingLimits = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::SetRenderTwistLimits(bool bEnable)
{
  if (m_bRenderTwistLimits == bEnable)
    return;

  m_bRenderTwistLimits = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::SetRenderPreviewMesh(bool bEnable)
{
  if (m_bRenderPreviewMesh == bEnable)
    return;

  m_bRenderPreviewMesh = bEnable;

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void xiiSkeletonAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // expose all the bones as parameters
  // such that we can create components that modify these bones

  auto*                 desc           = GetProperties();
  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);


  {
    xiiExposedBone bone;
    bone.m_sName = "<root-transform>";
    if (desc->GetRootTransform(bone.m_Transform).Failed())
    {
      XII_DEFAULT_DELETE(pExposedParams);
      xiiLog::Warning("Cannot expose bones with invalid skeleton import coordinates.");
      return;
    }

    xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
    param->m_sName             = "<root-transform>";
    param->m_DefaultValue.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);
  }

  struct Entry
  {
    XII_DECLARE_POD_TYPE();
    const xiiEditableSkeletonJoint* m_pJoint;
    const xiiEditableSkeletonJoint* m_pParent;
  };
  xiiDynamicArray<Entry>                      pending;
  xiiHashSet<const xiiEditableSkeletonJoint*> visited;
  for (xiiUInt32 i = desc->m_Children.GetCount(); i > 0; --i)
    pending.PushBack({desc->m_Children[i - 1], nullptr});
  while (!pending.IsEmpty())
  {
    const auto entry = pending.PeekBack();
    pending.PopBack();
    const auto* pJoint = entry.m_pJoint;
    if (pJoint == nullptr || visited.Insert(pJoint))
    {
      XII_DEFAULT_DELETE(pExposedParams);
      xiiLog::Warning("Cannot expose bones with invalid skeleton ownership.");
      return;
    }
    xiiExposedBone bone;
    bone.m_sName               = pJoint->GetName();
    bone.m_sParent             = entry.m_pParent != nullptr ? entry.m_pParent->GetName() : "";
    bone.m_Transform           = pJoint->m_LocalTransform;
    xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
    param->m_sName             = pJoint->GetName();
    param->m_DefaultValue.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());
    pExposedParams->m_Parameters.PushBack(param);
    for (xiiUInt32 i = pJoint->m_Children.GetCount(); i > 0; --i)
      pending.PushBack({pJoint->m_Children[i - 1], pJoint});
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

xiiTransformStatus xiiSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  XII_SUCCEED_OR_RETURN(SendPreviewConfiguration());

  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails wouldn't look as desired, for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    xiiSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload  = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }

  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiSkeletonAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkeletonAssetDocumentGenerator::xiiSkeletonAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

xiiSkeletonAssetDocumentGenerator::~xiiSkeletonAssetDocumentGenerator() = default;

void xiiSkeletonAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::Undecided;
    info.m_sName                                = "SkeletonImport";
    info.m_sIcon                                = ":/AssetIcons/Skeleton.svg";
  }
}

xiiStatus xiiSkeletonAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  xiiDocument* pDoc = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);
  if (pDoc == nullptr)
    return xiiStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  xiiSkeletonAssetDocument* pAssetDoc = xiiDynamicCast<xiiSkeletonAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", sInputFileRel.GetView());

  return XII_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiEditableSkeleton_1_2 : public xiiGraphPatch
{
public:
  xiiEditableSkeleton_1_2() :
    xiiGraphPatch("xiiEditableSkeleton", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

xiiEditableSkeleton_1_2 g_xiiEditableSkeleton_1_2;
