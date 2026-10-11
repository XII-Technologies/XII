/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter/AnimationImport.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRootMotionSource, 1)
  XII_ENUM_CONSTANTS(xiiRootMotionSource::None, xiiRootMotionSource::Constant)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetProperties, 4, xiiRTTIDefaultAllocator<xiiAnimationClipAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new xiiFileBrowserAttribute("Select Animation", xiiFileBrowserAttribute::MeshesWithAnimations)),
    XII_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", xiiDependencyFlags::None)),
    XII_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract),
    XII_ARRAY_MEMBER_PROPERTY("AvailableClips", m_AvailableClips)->AddAttributes(new xiiReadOnlyAttribute, new xiiContainerAttribute(false, false, false)),
    XII_MEMBER_PROPERTY("SampleRate", m_fSampleRate)->AddAttributes(new xiiDefaultValueAttribute(30.0f), new xiiClampValueAttribute(1.0f, 1000.0f)),
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipAssetDocument, 6, xiiRTTINoAllocator)
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

  xiiModelAnimationImportOptions options;
  options.m_sClipName    = pProp->m_sAnimationClipToExtract;
  options.m_fSampleRate  = pProp->m_fSampleRate;
  options.m_uiFirstFrame = pProp->m_uiFirstFrame;
  options.m_uiNumFrames  = pProp->m_uiNumFrames;
  options.m_bAdditive    = pProp->m_bAdditive;
  xiiModelAnimationImport imported;
  xiiStringBuilder        error;
  const auto              result = xiiModelAnimationImport::ReadFile(sAbsFilename, options, imported, &error);
  // Clip discovery remains useful when an authored selection no longer exists.
  // Read-only source metadata still belongs to the document and must not be
  // mutated by a background transform.
  if (!imported.m_AvailableClips.IsEmpty() && imported.m_AvailableClips != pProp->m_AvailableClips)
  {
    if (transformFlags.IsSet(xiiTransformFlags::BackgroundProcessing))
      return xiiTransformStatus(xiiTransformResult::NeedsImport);
    GetObjectAccessor()->StartTransaction("Update Available Animation Clips");
    pProp->m_AvailableClips = imported.m_AvailableClips;
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
    pProp = GetProperties();
  }
  if (result.Failed())
    return xiiStatus(error.GetView());
  if (pProp->m_RootMotionMode == xiiRootMotionSource::Constant && pProp->m_vConstantRootMotion != xiiVec3::MakeZero())
    return xiiStatus("The native animation runtime does not support authored constant root-motion velocity. The setting is preserved; choose None to transform this clip without root motion.");
  if (pProp->m_RootMotionMode != xiiRootMotionSource::None && pProp->m_RootMotionMode != xiiRootMotionSource::Constant)
    return xiiStatus("Unknown root-motion source.");

  range.BeginNextStep("Writing Result");
  desc = std::move(imported.m_Clip);
  for (const auto& point : pProp->m_EventTrack.m_ControlPoints)
  {
    const double time = point.GetTickAsTime().GetSeconds();
    // Authored timeline events are relative to the resulting clip. Retain source
    // data outside the selected duration in the document, without firing it.
    if (time < 0 || time > desc.m_Duration.GetSeconds())
      continue;
    if (point.m_sEvent.IsEmpty())
      return xiiStatus("Animation events require a nonempty name.");
    auto& event    = desc.m_Events.ExpandAndGetRef();
    event.m_fTime  = static_cast<float>(time);
    event.m_sEvent = point.m_sEvent;
  }
  xiiSorting::InsertionSort(desc.m_Events, [](const xiiAnimationClipEvent& a, const xiiAnimationClipEvent& b) { return a.m_fTime < b.m_fTime; });
  desc.ComputeRuntimeHash();
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_SUCCEED_OR_RETURN(desc.Serialize(writer));
  return storage.CopyToStream(stream);
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
  if (sMode != "AnimationClipImport_Single" && sMode != "AnimationClipImport_All")
    return xiiStatus("Unknown animation clip import mode.");
  xiiDynamicArray<xiiString> clips;
  xiiStringBuilder           error;
  if (xiiModelAnimationImport::ReadClipNames(sInputFileAbs, clips, &error).Failed())
    return xiiStatus(error.GetView());
  auto*            app = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder sourceRelative(sInputFileAbs);
  app->MakePathDataDirectoryRelative(sourceRelative);
  xiiStringBuilder title;
  title.SetFormat("Select Preview Mesh for Animation Clip '{}'", sInputFileAbs.GetFileName());
  xiiStringBuilder     previewMesh;
  xiiQtAssetBrowserDlg dialog(nullptr, xiiUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skinned", title);
  if (dialog.exec() != 0 && dialog.GetSelectedAssetGuid().IsValid())
    xiiConversionUtils::ToString(dialog.GetSelectedAssetGuid(), previewMesh);

  xiiHybridArray<xiiDocument*, 16> created;
  bool                             complete = false;
  XII_SCOPE_EXIT({
    if (!complete)
    {
      for (auto* document : created)
      {
        const xiiString path = document->GetDocumentPath();
        document->GetDocumentManager()->CloseDocument(document);
        if (xiiOSFile::DeleteFile(path).Failed())
          xiiLog::Error("Could not remove failed animation import asset '{}'.", path);
        xiiAssetCurator::GetSingleton()->NotifyOfFileChange(path);
      }
    }
  });
  const xiiUInt32 count = sMode == "AnimationClipImport_All" ? clips.GetCount() : 1;
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    xiiStringBuilder path(sInputFileAbs);
    path.ChangeFileExtension(GetDocumentExtension());
    if (sMode == "AnimationClipImport_All")
    {
      xiiStringBuilder name;
      xiiPathUtils::MakeValidFilename(clips[i], '_', name);
      name.ReplaceAll(" ", "-");
      name.Prepend(path.GetFileName(), "_");
      path.ChangeFileName(name);
    }
    xiiOSFile::FindFreeFilename(path);
    auto* document = app->CreateDocument(path, xiiDocumentFlags::None);
    if (document == nullptr)
      return xiiStatus(xiiFmt("Could not create animation clip asset '{}'.", path));
    created.PushBack(document);
    auto* asset = xiiDynamicCast<xiiAnimationClipAssetDocument*>(document);
    if (asset == nullptr)
      return xiiStatus("Created animation document has an unexpected type.");
    auto* accessor = asset->GetObjectAccessor();
    accessor->StartTransaction("Import Animation Clip");
    bool configured = false;
    XII_SCOPE_EXIT(if (!configured) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "File", sourceRelative.GetView()));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "PreviewMesh", previewMesh.GetView()));
    if (sMode == "AnimationClipImport_All")
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "UseAnimationClip", clips[i].GetView()));
    accessor->FinishTransaction();
    configured = true;
  }
  out_generatedDocuments.PushBackRange(created);
  complete = true;
  return XII_SUCCESS;
}
