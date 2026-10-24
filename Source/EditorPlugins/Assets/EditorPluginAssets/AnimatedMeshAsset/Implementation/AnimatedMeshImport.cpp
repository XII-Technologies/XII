/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/Util/MeshTextureImport.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <ModelImporter/AnimationImport.h>
#include <ModelImporter/SkeletonImport.h>

xiiStatus xiiAnimatedMeshAssetDocumentGenerator::GenerateComplete(xiiStringView sInputFileAbs, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  // Inspect before creating documents. Reuse the same native import path used by
  // transforms, so a successful group cannot start with an unimportable mesh/rig.
  xiiModelMeshImportOptions options;
  options.m_bImportSkinning      = true;
  options.m_bRecalculateTangents = false;
  xiiModelMeshImport         mesh;
  xiiModelSkeletonImport     skeleton;
  xiiDynamicArray<xiiString> clips;
  xiiStringBuilder           error;
  if (xiiModelMeshImport::ReadFile(sInputFileAbs, options, mesh, &error).Failed() ||
      xiiModelSkeletonImport::ReadFile(sInputFileAbs, skeleton, &error).Failed() ||
      xiiModelAnimationImport::ReadClipNames(sInputFileAbs, clips, &error).Failed())
    return xiiStatus(error.GetView());

  auto*            app = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder source(sInputFileAbs);
  if (!app->MakePathDataDirectoryRelative(source))
    return xiiStatus("The model must be inside a project data directory.");

  xiiHybridArray<xiiDocument*, 16> created;
  bool                             complete = false;
  XII_SCOPE_EXIT(if (!complete) {
    for (auto* document : created)
    {
      const xiiString path(document->GetDocumentPath());
      document->GetDocumentManager()->CloseDocument(document);
      if (xiiOSFile::DeleteFile(path).Failed())
        xiiLog::Error("Could not remove failed model import asset '{}'.", path);
      xiiAssetCurator::GetSingleton()->NotifyOfFileChange(path);
    }
  });
  // Existing assets retain authored settings and identity. Newly created documents
  // are returned in dependency order for the generator's normal save/close workflow.
  auto create = [&](xiiStringView path, xiiDocument*& document, xiiString& id) -> xiiStatus {
    document = nullptr;
    if (xiiOSFile::ExistsFile(path))
      return xiiMeshImportUtils::ResolveAssetReference(path, id);
    document = app->CreateDocument(path, xiiDocumentFlags::None);
    if (document == nullptr)
      return xiiStatus(xiiFmt("Could not create model import asset '{}'.", path));
    created.PushBack(document);
    xiiStringBuilder guid;
    id = xiiConversionUtils::ToString(document->GetGuid(), guid);
    return XII_SUCCESS;
  };
  auto configure = [](xiiDocument& document, xiiStringView sourceProperty, xiiStringView sourcePath, xiiStringView linkProperty, xiiStringView link) -> xiiStatus {
    auto* accessor   = document.GetObjectAccessor();
    auto* properties = document.GetObjectManager()->GetRootObject()->GetChildren()[0];
    accessor->StartTransaction("Import Model Asset Group");
    bool configured = false;
    XII_SCOPE_EXIT(if (!configured) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, sourceProperty, sourcePath));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, linkProperty, link));
    accessor->FinishTransaction();
    configured = true;
    return XII_SUCCESS;
  };

  xiiStringBuilder meshPath(sInputFileAbs), skeletonPath(sInputFileAbs);
  meshPath.ChangeFileExtension(GetDocumentExtension());
  skeletonPath.ChangeFileExtension("xiiSkeletonAsset");
  xiiString    meshId, skeletonId;
  xiiDocument* skeletonDocument = nullptr;
  xiiDocument* meshDocument     = nullptr;
  XII_SUCCEED_OR_RETURN(create(skeletonPath, skeletonDocument, skeletonId));
  XII_SUCCEED_OR_RETURN(create(meshPath, meshDocument, meshId));
  if (skeletonDocument != nullptr)
  {
    XII_SUCCEED_OR_RETURN(configure(*skeletonDocument, "File", source, "PreviewMesh", meshId));
    // Animated meshes and clips use source-space joint transforms. Choose the
    // equivalent identity basis for their generated skeleton instead of applying
    // the standalone skeleton importer's default coordinate rotation a second time.
    auto* asset    = xiiStaticCast<xiiSkeletonAssetDocument*>(skeletonDocument);
    auto* accessor = asset->GetObjectAccessor();
    accessor->StartTransaction("Match Model Skeleton Coordinates");
    bool configured = false;
    XII_SCOPE_EXIT(if (!configured) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "ImportTransform", static_cast<xiiInt32>(xiiMeshImportTransform::Custom)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "RightDir", static_cast<xiiInt32>(xiiBasisAxis::PositiveY)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "UpDir", static_cast<xiiInt32>(xiiBasisAxis::PositiveZ)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "FlipForwardDir", true));
    accessor->FinishTransaction();
    configured = true;
  }
  if (meshDocument != nullptr)
    XII_SUCCEED_OR_RETURN(configure(*meshDocument, "MeshFile", source, "DefaultSkeleton", skeletonId));

  for (const auto& clip : clips)
  {
    xiiStringBuilder name, path(sInputFileAbs);
    xiiPathUtils::MakeValidFilename(clip, '_', name);
    // Hash the complete clip label: sanitization and case folding cannot collide.
    name.Prepend(sInputFileAbs.GetFileName(), "_");
    name.AppendFormat("_{}", xiiArgU(xiiHashingUtils::StringHash(clip), 16, true, 16));
    path.ChangeFileNameAndExtension(xiiStringBuilder(name, ".xiiAnimationClipAsset"));
    xiiDocument* document = nullptr;
    xiiString    id;
    XII_SUCCEED_OR_RETURN(create(path, document, id));
    if (document == nullptr)
      continue;
    XII_SUCCEED_OR_RETURN(configure(*document, "File", source, "PreviewMesh", meshId));
    auto* asset    = xiiStaticCast<xiiAnimationClipAssetDocument*>(document);
    auto* accessor = asset->GetObjectAccessor();
    accessor->StartTransaction("Select Imported Animation Clip");
    bool configured = false;
    XII_SCOPE_EXIT(if (!configured) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(asset->GetPropertyObject(), "UseAnimationClip", clip.GetView()));
    for (const auto& label : clips)
      XII_SUCCEED_OR_RETURN(accessor->InsertValueByName(asset->GetPropertyObject(), "AvailableClips", label.GetView(), -1));
    accessor->FinishTransaction();
    configured = true;
  }
  out_generatedDocuments.PushBackRange(created);
  complete = true;
  return XII_SUCCESS;
}
