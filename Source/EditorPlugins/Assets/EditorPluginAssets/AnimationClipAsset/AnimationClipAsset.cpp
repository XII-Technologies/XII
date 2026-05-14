/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter/ModelImporter.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRootMotionSource, 1)
  XII_ENUM_CONSTANTS(xiiRootMotionSource::None, xiiRootMotionSource::Constant)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetProperties, 3, xiiRTTIDefaultAllocator<xiiAnimationClipAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new xiiFileBrowserAttribute("Select Animation", xiiFileBrowserAttribute::MeshesWithAnimations)),
    XII_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", xiiDependencyFlags::None)),
    XII_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract),
    XII_ARRAY_MEMBER_PROPERTY("AvailableClips", m_AvailableClips)->AddAttributes(new xiiReadOnlyAttribute, new xiiContainerAttribute(false, false, false)),
    XII_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    XII_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    XII_MEMBER_PROPERTY("Additive", m_bAdditive),
    XII_ENUM_MEMBER_PROPERTY("RootMotion", xiiRootMotionSource, m_RootMotionMode),
    XII_MEMBER_PROPERTY("ConstantRootMotion", m_vConstantRootMotion),
    XII_MEMBER_PROPERTY("EventTrack", m_EventTrack)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimationClipAssetProperties::xiiAnimationClipAssetProperties()  = default;
xiiAnimationClipAssetProperties::~xiiAnimationClipAssetProperties() = default;

void xiiAnimationClipAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != xiiGetStaticRTTI<xiiAnimationClipAssetProperties>())
    return;

  auto& props = *e.m_pPropertyStates;

  const xiiInt64 motionType = e.m_pObject->GetTypeAccessor().GetValue("RootMotion").ConvertTo<xiiInt64>();

  switch (motionType)
  {
    case xiiRootMotionSource::Constant:
      props["ConstantRootMotion"].m_Visibility = xiiPropertyUiState::Default;
      break;

    default:
      props["ConstantRootMotion"].m_Visibility = xiiPropertyUiState::Invisible;
      break;
  }
}

xiiAnimationClipAssetDocument::xiiAnimationClipAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiAnimationClipAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

void xiiAnimationClipAssetDocument::SetCommonAssetUiState(xiiCommonAssetUiState::Enum state, double value)
{
  switch (state)
  {
    case xiiCommonAssetUiState::SimulationSpeed:
      m_fSimulationSpeed = value;
      break;
    default:
      break;
  }

  // handles standard booleans and broadcasts the event
  return SUPER::SetCommonAssetUiState(state, value);
}

double xiiAnimationClipAssetDocument::GetCommonAssetUiState(xiiCommonAssetUiState::Enum state) const
{
  switch (state)
  {
    case xiiCommonAssetUiState::SimulationSpeed:
      return m_fSimulationSpeed;
    default:
      break;
  }

  return SUPER::GetCommonAssetUiState(state);
}

xiiTransformStatus xiiAnimationClipAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiProgressRange range("Transforming Asset", 2, false);

  xiiAnimationClipAssetProperties* pProp = GetProperties();

  xiiAnimationClipResourceDescriptor desc;

  range.BeginNextStep("Importing Animations");

  xiiStringBuilder sAbsFilename = pProp->m_sSourceFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return xiiStatus(xiiFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  xiiUniquePtr<xiiModelImporter::Importer> pImporter = xiiModelImporter::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return xiiStatus("No known importer for this file type.");

  xiiEditableSkeleton skeleton;

  xiiModelImporter::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  // opt.m_pSkeletonOutput = &skeleton; // TODO: may be needed later to optimize the clip
  opt.m_pAnimationOutput    = &desc;
  opt.m_bAdditiveAnimation  = pProp->m_bAdditive;
  opt.m_sAnimationToImport  = pProp->m_sAnimationClipToExtract;
  opt.m_uiFirstAnimKeyframe = pProp->m_uiFirstFrame;
  opt.m_uiNumAnimKeyframes  = pProp->m_uiNumFrames;

  const xiiResult res = pImporter->Import(opt);

  if (res.Succeeded())
  {
    if (pProp->m_RootMotionMode == xiiRootMotionSource::Constant)
    {
      desc.m_vConstantRootMotion = pProp->m_vConstantRootMotion;
    }

    range.BeginNextStep("Writing Result");

    pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

    XII_SUCCEED_OR_RETURN(desc.Serialize(stream));
  }

  // if we found information about animation clips, update the UI, even if the transform failed
  if (!pImporter->m_OutputAnimationNames.IsEmpty())
  {
    pProp->m_AvailableClips.SetCount(pImporter->m_OutputAnimationNames.GetCount());
    for (xiiUInt32 clip = 0; clip < pImporter->m_OutputAnimationNames.GetCount(); ++clip)
    {
      pProp->m_AvailableClips[clip] = pImporter->m_OutputAnimationNames[clip];
    }

    // merge the new data with the actual asset document
    ApplyNativePropertyChangesToObjectManager(true);
  }

  if (res.Failed())
    return xiiStatus("Model importer was unable to read this asset.");

  return XII_SUCCESS;
}

xiiTransformStatus xiiAnimationClipAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails would remain black for assets transformed in the background
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

xiiUuid xiiAnimationClipAssetDocument::InsertEventTrackCpAt(xiiInt64 iTickX, const char* szValue)
{
  xiiObjectCommandAccessor accessor(GetCommandHistory());
  xiiObjectAccessorBase&   acc = accessor;
  acc.StartTransaction("Insert Event");

  const xiiAbstractProperty* pTrackProp = xiiGetStaticRTTI<xiiAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  xiiUuid                    trackGuid  = accessor.Get<xiiUuid>(GetPropertyObject(), pTrackProp);

  xiiUuid newObjectGuid;
  XII_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, xiiGetStaticRTTI<xiiEventTrackControlPointData>(), newObjectGuid).Succeeded(), "");
  const xiiDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  XII_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  XII_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

// void xiiAnimationClipAssetDocument::ApplyCustomRootMotion(xiiAnimationClipResourceDescriptor& anim) const
//{
//  const xiiAnimationClipAssetProperties* pProp = GetProperties();
//  const xiiUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  xiiArrayPtr<xiiTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const xiiVec3 vKeyframeMotion = pProp->m_vCustomRootMotion / (float)anim.GetFramesPerSecond();
//  const xiiTransform rootTransform(vKeyframeMotion);
//
//  for (xiiUInt32 kf = 0; kf < anim.GetNumFrames(); ++kf)
//  {
//    pRootTransforms[kf] = rootTransform;
//  }
//}
//
// void xiiAnimationClipAssetDocument::ExtractRootMotionFromFeet(xiiAnimationClipResourceDescriptor& anim, const xiiSkeleton& skeleton) const
//{
//  const xiiAnimationClipAssetProperties* pProp = GetProperties();
//  const xiiUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  xiiArrayPtr<xiiTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const xiiUInt16 uiFoot1 = skeleton.FindJointByName(xiiTempHashedString(pProp->m_sJoint1.GetData()));
//  const xiiUInt16 uiFoot2 = skeleton.FindJointByName(xiiTempHashedString(pProp->m_sJoint2.GetData()));
//
//  if (uiFoot1 == xiiInvalidJointIndex || uiFoot2 == xiiInvalidJointIndex)
//  {
//    xiiLog::Error("Joints '{0}' and '{1}' could not be found in animation clip", pProp->m_sJoint1, pProp->m_sJoint2);
//    return;
//  }
//
//  xiiAnimationPose pose;
//  pose.Configure(skeleton);
//
//  xiiVec3 lastFootPos1(0), lastFootPos2(0);
//
//  // init last foot position with very last frame data
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, anim.GetNumFrames() - 1);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    lastFootPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    lastFootPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//  }
//
//  xiiInt32 lastFootDown = (lastFootPos1.z < lastFootPos2.z) ? 1 : 2;
//
//  xiiHybridArray<xiiUInt16, 32> unknownMotion;
//
//  for (xiiUInt16 frame = 0; frame < anim.GetNumFrames(); ++frame)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, frame);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    const xiiVec3 footPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    const xiiVec3 footPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//
//    const xiiVec3 footDir1 = footPos1 - lastFootPos1;
//    const xiiVec3 footDir2 = footPos2 - lastFootPos2;
//
//    xiiVec3 rootMotion(0);
//
//    const xiiInt32 curFootDown = (footPos1.z < footPos2.z) ? 1 : 2;
//
//    if (lastFootDown == curFootDown)
//    {
//      if (curFootDown == 1)
//        rootMotion = -footDir1;
//      else
//        rootMotion = -footDir2;
//
//      rootMotion.z = 0;
//      pRootTransforms[frame] = xiiTransform(rootMotion);
//    }
//    else
//    {
//      // set them via average later on
//      unknownMotion.PushBack(frame);
//      pRootTransforms[frame].SetIdentity();
//    }
//
//    lastFootDown = curFootDown;
//    lastFootPos1 = footPos1;
//    lastFootPos2 = footPos2;
//  }
//
//  // fix unknown motion frames
//  for (xiiUInt16 crossedFeet : unknownMotion)
//  {
//    const xiiUInt16 prevFrame = (crossedFeet > 0) ? (crossedFeet - 1) : anim.GetNumFrames() - 1;
//    const xiiUInt16 nextFrame = (crossedFeet + 1) % anim.GetNumFrames();
//
//    const xiiVec3 avgTranslation = xiiMath::Lerp(pRootTransforms[prevFrame].m_vPosition, pRootTransforms[nextFrame].m_vPosition, 0.5f);
//
//    pRootTransforms[crossedFeet] = xiiTransform(avgTranslation);
//  }
//
//  const xiiUInt16 numFrames = anim.GetNumFrames();
//
//  xiiHybridArray<xiiVec3, 32> translations;
//  translations.SetCount(numFrames);
//
//  for (xiiUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    translations[thisFrame] = pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  // do some smoothing
//  for (xiiUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    const xiiUInt16 prevFrame2 = (numFrames + thisFrame - 2) % numFrames;
//    const xiiUInt16 prevFrame = (numFrames + thisFrame - 1) % numFrames;
//    const xiiUInt16 nextFrame = (thisFrame + 1) % numFrames;
//    const xiiUInt16 nextFrame2 = (thisFrame + 2) % numFrames;
//
//    const xiiVec3 smoothedTranslation =
//      (translations[prevFrame2] + translations[prevFrame] + translations[thisFrame] + translations[nextFrame] + translations[nextFrame2]) * 0.2f;
//
//    pRootTransforms[thisFrame].m_vPosition = smoothedTranslation;
//  }
//
//  // for (xiiUInt32 i = 0; i < anim.GetNumFrames(); ++i)
//  //{
//  //  xiiLog::Info("Motion {0}: {1} | {2}", xiiArgI(i, 3), xiiArgF(pRootTransforms[i].m_vPosition.x, 1),
//  //              xiiArgF(pRootTransforms[i].m_vPosition.y, 1));
//  //}
//}
//
// void xiiAnimationClipAssetDocument::MakeRootMotionConstantAverage(xiiAnimationClipResourceDescriptor& anim) const
//{
//  const xiiUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  xiiArrayPtr<xiiTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//  const xiiUInt16 numFrames = anim.GetNumFrames();
//
//  xiiVec3 avgFootTranslation(0);
//
//  for (xiiUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    avgFootTranslation += pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  avgFootTranslation /= numFrames;
//
//  for (xiiUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    pRootTransforms[thisFrame].m_vPosition = avgFootTranslation;
//  }
//}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiAnimationClipAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimationClipAssetDocumentGenerator::xiiAnimationClipAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

xiiAnimationClipAssetDocumentGenerator::~xiiAnimationClipAssetDocumentGenerator() = default;

void xiiAnimationClipAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::Undecided;
    info.m_sName                                = "AnimationClipImport_Single";
    info.m_sIcon                                = ":/AssetIcons/Animation_Clip.svg";
  }

  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::Undecided;
    info.m_sName                                = "AnimationClipImport_All";
    info.m_sIcon                                = ":/AssetIcons/Animation_Clip.svg";
  }
}

xiiStatus xiiAnimationClipAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  xiiStringBuilder title;
  title.SetFormat("Select Preview Mesh for Animation Clip '{}'", sInputFileAbs.GetFileName());

  xiiStringBuilder sPreviewMesh;

  xiiQtAssetBrowserDlg dlg(nullptr, xiiUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skinned", title);
  if (dlg.exec() != 0)
  {
    if (dlg.GetSelectedAssetGuid().IsValid())
    {
      xiiConversionUtils::ToString(dlg.GetSelectedAssetGuid(), sPreviewMesh);
    }
  }

  if (sMode == "AnimationClipImport_Single")
  {
    xiiDocument* pDoc = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);
    if (pDoc == nullptr)
      return xiiStatus("Could not create target document");

    out_generatedDocuments.PushBack(pDoc);

    xiiAnimationClipAssetDocument* pAssetDoc = xiiDynamicCast<xiiAnimationClipAssetDocument*>(pDoc);

    auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
    accessor.SetValue("File", sInputFileRel.GetView());
    accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

    return XII_SUCCESS;
  }

  if (sMode == "AnimationClipImport_All")
  {
    xiiModelImporter::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;

    xiiUniquePtr<xiiModelImporter::Importer> pImporter = xiiModelImporter::RequestImporterForFileType(opt.m_sSourceFile);
    if (pImporter == nullptr)
      return xiiStatus("No known importer for this file type.");

    if (pImporter->Import(opt).Failed())
      return xiiStatus("Failed to import asset.");

    xiiStringBuilder sFilename;
    xiiStringBuilder sOutFile2;

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      sFilename = clip;
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);
      xiiOSFile::FindFreeFilename(sOutFile2);

      xiiDocument* pDoc = pApp->CreateDocument(sOutFile2, xiiDocumentFlags::None);
      if (pDoc == nullptr)
        return xiiStatus("Could not create target document");

      out_generatedDocuments.PushBack(pDoc);

      xiiAnimationClipAssetDocument* pAssetDoc = xiiDynamicCast<xiiAnimationClipAssetDocument*>(pDoc);

      auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
      accessor.SetValue("File", sInputFileRel.GetView());
      accessor.SetValue("UseAnimationClip", clip);
      accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());
    }

    return XII_SUCCESS;
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiStatus(XII_FAILURE);
}
