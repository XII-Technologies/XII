#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonAssetDocument, 10, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiTransform CalculateTransformationMatrix(const xiiEditableSkeleton* pProp)
{
  const float us = xiiMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const xiiBasisAxis::Enum rightDir   = pProp->m_RightDir;
  const xiiBasisAxis::Enum upDir      = pProp->m_UpDir;
  xiiBasisAxis::Enum       forwardDir = xiiBasisAxis::GetOrthogonalAxis(rightDir, upDir, !pProp->m_bFlipForwardDir);

  xiiTransform t;
  t.SetIdentity();
  t.m_vScale.Set(us);

  // prevent mirroring in the rotation matrix, because we can't generate a quaternion from that
  if (!pProp->m_bFlipForwardDir)
  {
    switch (forwardDir)
    {
      case xiiBasisAxis::PositiveX:
        forwardDir = xiiBasisAxis::NegativeX;
        t.m_vScale.x *= -1;
        break;
      case xiiBasisAxis::PositiveY:
        forwardDir = xiiBasisAxis::NegativeY;
        t.m_vScale.y *= -1;
        break;
      case xiiBasisAxis::PositiveZ:
        forwardDir = xiiBasisAxis::NegativeZ;
        t.m_vScale.z *= -1;
        break;
      case xiiBasisAxis::NegativeX:
        forwardDir = xiiBasisAxis::PositiveX;
        t.m_vScale.x *= -1;
        break;
      case xiiBasisAxis::NegativeY:
        forwardDir = xiiBasisAxis::PositiveY;
        t.m_vScale.y *= -1;
        break;
      case xiiBasisAxis::NegativeZ:
        forwardDir = xiiBasisAxis::PositiveZ;
        t.m_vScale.z *= -1;
        break;
    }
  }

  xiiMat3 rot = xiiBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, 1.0f);
  t.m_qRotation.SetFromMat3(rot);

  return t;
}

xiiSkeletonAssetDocument::xiiSkeletonAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiEditableSkeleton>(szDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiSkeletonAssetDocument::~xiiSkeletonAssetDocument() = default;

void xiiSkeletonAssetDocument::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiEditableSkeletonJoint>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool                       overrideSurface        = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool                       overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();
    const xiiSkeletonJointType::Enum jointType              = (xiiSkeletonJointType::Enum)e.m_pObject->GetTypeAccessor().GetValue("JointType").ConvertTo<xiiInt32>();

    props["Surface"].m_Visibility               = overrideSurface ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["CollisionLayer"].m_Visibility        = overrideCollisionLayer ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["LimitSwing"].m_Visibility            = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["SwingLimitY"].m_Visibility           = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["SwingLimitZ"].m_Visibility           = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["LimitTwist"].m_Visibility            = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["TwistLimitHalfAngle"].m_Visibility   = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["TwistLimitCenterAngle"].m_Visibility = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["LocalRotation"].m_Visibility         = (jointType == xiiSkeletonJointType::SwingTwist) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;

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
  desc.m_RootTransform = CalculateTransformationMatrix(&skeleton);
  skeleton.FillResourceDescriptor(desc);

  XII_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return xiiStatus(XII_SUCCESS);
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

void xiiSkeletonAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // expose all the bones as parameters
  // such that we can create components that modify these bones

  auto*                 desc           = GetProperties();
  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);


  {
    xiiExposedBone bone;
    bone.m_sName     = "<root-transform>";
    bone.m_Transform = CalculateTransformationMatrix(desc);

    xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
    param->m_sName             = "<root-transform>";
    param->m_DefaultValue.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);
  }

  auto Traverse = [&](xiiEditableSkeletonJoint* pJoint, const char* szParent, auto recurse) -> void {
    xiiExposedBone bone;
    bone.m_sName     = pJoint->GetName();
    bone.m_sParent   = szParent;
    bone.m_Transform = pJoint->m_LocalTransform;

    xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
    param->m_sName             = pJoint->GetName();
    param->m_DefaultValue.CopyTypedObject(&bone, xiiGetStaticRTTI<xiiExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);

    for (auto pChild : pJoint->m_Children)
    {
      recurse(pChild, pJoint->GetName(), recurse);
    }
  };

  for (auto ptr : desc->m_Children)
  {
    Traverse(ptr, "", Traverse);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

xiiTransformStatus xiiSkeletonAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    XII_SCOPE_EXIT(m_bIsTransforming = false);

    xiiProgressRange range("Transforming Asset", 3, false);

    xiiEditableSkeleton* pProp = GetProperties();

    xiiStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (sAbsFilename.IsEmpty())
    {
      range.BeginNextStep("Writing Result");
      XII_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties()));
    }
    else
    {
      if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
      {
        return xiiStatus(xiiFmt("Couldn't make path absolute: '{0};", sAbsFilename));
      }

      xiiUniquePtr<xiiModelImporter2::Importer> pImporter = xiiModelImporter2::RequestImporterForFileType(sAbsFilename);
      if (pImporter == nullptr)
        return xiiStatus("No known importer for this file type.");

      range.BeginNextStep("Importing Source File");

      xiiEditableSkeleton newSkeleton;

      xiiModelImporter2::ImportOptions opt;
      opt.m_sSourceFile     = sAbsFilename;
      opt.m_pSkeletonOutput = &newSkeleton;

      if (pImporter->Import(opt).Failed())
        return xiiStatus("Model importer was unable to read this asset.");

      range.BeginNextStep("Importing Skeleton Data");

      // synchronize the old data (collision geometry etc.) with the new hierarchy
      const xiiEditableSkeleton* pFinalSkeleton = MergeWithNewSkeleton(newSkeleton);

      range.BeginNextStep("Writing Result");
      XII_SUCCEED_OR_RETURN(WriteResource(stream, *pFinalSkeleton));

      // merge the new data with the actual asset document
      ApplyNativePropertyChangesToObjectManager(true);
    }
  }

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return xiiStatus(XII_SUCCESS);
}

xiiTransformStatus xiiSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

const xiiEditableSkeleton* xiiSkeletonAssetDocument::MergeWithNewSkeleton(xiiEditableSkeleton& newSkeleton)
{
  xiiEditableSkeleton*                               pOldSkeleton = GetProperties();
  xiiMap<xiiString, const xiiEditableSkeletonJoint*> prevJoints;

  // map all old joints by name
  {
    auto TraverseJoints = [&prevJoints](const auto& self, xiiEditableSkeletonJoint* pJoint) -> void {
      prevJoints[pJoint->GetName()] = pJoint;

      for (xiiEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild);
      }
    };

    for (xiiEditableSkeletonJoint* pChild : pOldSkeleton->m_Children)
    {
      TraverseJoints(TraverseJoints, pChild);
    }
  }

  // copy old properties to new skeleton
  {
    auto TraverseJoints = [&prevJoints](const auto& self, xiiEditableSkeletonJoint* pJoint, const xiiTransform& root, xiiTransform origin) -> void {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      // use the parent rotation as the gizmo base rotation
      xiiMat4 modelTransform, fullTransform;
      modelTransform = origin.GetAsMat4();
      xiiMsgAnimationPoseUpdated::ComputeFullBoneTransform(root.GetAsMat4(), modelTransform, fullTransform, pJoint->m_qGizmoOffsetRotationRO);

      origin.SetGlobalTransform(origin, pJoint->m_LocalTransform);
      pJoint->m_vGizmoOffsetPositionRO = root.TransformPosition(origin.m_vPosition);

      for (xiiEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild, root, origin);
      }
    };

    for (xiiEditableSkeletonJoint* pChild : newSkeleton.m_Children)
    {
      TraverseJoints(TraverseJoints, pChild, CalculateTransformationMatrix(pOldSkeleton), xiiTransform::IdentityTransform());
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = newSkeleton.m_Children;
  newSkeleton.m_Children.Clear(); // prevent this skeleton from deallocating the joints

  return pOldSkeleton;
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

void xiiSkeletonAssetDocumentGenerator::GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_modes) const
{
  xiiStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    xiiAssetDocumentGenerator::Info& info = out_modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::Undecided;
    info.m_sName                          = "SkeletonImport";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Skeleton.png";
  }
}

xiiStatus xiiSkeletonAssetDocumentGenerator::Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiSkeletonAssetDocument* pAssetDoc = xiiDynamicCast<xiiSkeletonAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiSkeletonAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", sDataDirRelativePath);

  return xiiStatus(XII_SUCCESS);
}
