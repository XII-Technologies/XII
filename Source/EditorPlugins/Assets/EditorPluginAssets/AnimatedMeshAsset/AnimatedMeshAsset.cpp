/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <ModelImporter/MeshImport.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetDocument, 10, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimatedMeshAssetDocument::xiiAnimatedMeshAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiAnimatedMeshAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiTransformStatus xiiAnimatedMeshAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiProgressRange range("Transforming Asset", 2, false);

  xiiAnimatedMeshAssetProperties* pProp = GetProperties();

  if (pProp->m_sDefaultSkeleton.IsEmpty())
  {
    return xiiStatus("Animated mesh doesn't have a default skeleton assigned.");
  }

  xiiMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  XII_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc, !transformFlags.IsSet(xiiTransformFlags::BackgroundProcessing)));

  // the properties object can get invalidated by the CreateMeshFromFile() call
  pProp = GetProperties();

  range.BeginNextStep("Writing Result");

  if (!pProp->m_sDefaultSkeleton.IsEmpty())
  {
    desc.m_hDefaultSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(pProp->m_sDefaultSkeleton);
  }

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  desc.Save(writer);
  return storage.CopyToStream(stream);
}

xiiTransformStatus xiiAnimatedMeshAssetDocument::CreateMeshFromFile(xiiAnimatedMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc, bool bAllowMaterialImport)
{
  xiiProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  xiiStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return xiiStatus(xiiFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  xiiModelMeshImportOptions options;
  options.m_bImportSkinning       = true;
  options.m_bNormalizeWeights     = pProp->m_bNormalizeWeights;
  options.m_bRecalculateNormals   = pProp->m_bRecalculateNormals;
  options.m_uiGeneratedLODs       = pProp->m_uiGeneratedLODs;
  options.m_bRecalculateTangents  = pProp->m_bRecalculateTangents;
  options.m_VertexColorConversion = pProp->m_VertexColorConversion;
  if (pProp->m_bSimplifyMesh)
  {
    options.m_fSimplification           = xiiMath::Min<xiiUInt32>(pProp->m_uiMeshSimplification, 99) / 100.0f;
    options.m_fMaxSimplificationError   = pProp->m_uiMaxSimplificationError / 100.0f;
    options.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }
  xiiModelMeshImport imported;
  xiiStringBuilder   error;
  if (xiiModelMeshImport::ReadFile(sAbsFilename, options, imported, &error).Failed())
    return xiiStatus(error.GetView());
  range.BeginNextStep("Importing Materials");
  auto materialSlots = pProp->m_Slots;
  XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::PrepareMaterialSlots(materialSlots, pProp->m_bImportMaterials, bAllowMaterialImport, GetDocumentPath(), sAbsFilename, imported));
  XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::CopyMeshAssetMaterialSlotToResource(imported.m_Mesh, materialSlots));
  if (!xiiMeshMaterialSlots::Equal(materialSlots, pProp->m_Slots))
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Materials");
    pProp->m_Slots = std::move(materialSlots);
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }
  desc = std::move(imported.m_Mesh);
  return XII_SUCCESS;
}

xiiTransformStatus xiiAnimatedMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}


//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiAnimatedMeshAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimatedMeshAssetDocumentGenerator::xiiAnimatedMeshAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

xiiAnimatedMeshAssetDocumentGenerator::~xiiAnimatedMeshAssetDocumentGenerator() = default;

void xiiAnimatedMeshAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    auto& info      = out_modes.ExpandAndGetRef();
    info.m_Priority = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName    = "AnimatedMeshImport.Complete";
    info.m_sIcon    = ":/AssetIcons/Animated_Mesh.svg";
  }
  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                                = "AnimatedMeshImport.WithMaterials";
    info.m_sIcon                                = ":/AssetIcons/Animated_Mesh.svg";
  }

  {
    xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority                             = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                                = "AnimatedMeshImport.NoMaterials";
    info.m_sIcon                                = ":/AssetIcons/Animated_Mesh.svg";
  }
}

xiiStatus xiiAnimatedMeshAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  if (sMode == "AnimatedMeshImport.Complete")
    return GenerateComplete(sInputFileAbs, out_generatedDocuments);
  if (sMode != "AnimatedMeshImport.WithMaterials" && sMode != "AnimatedMeshImport.NoMaterials")
    return xiiStatus("Unknown animated mesh import mode.");
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

  xiiAnimatedMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiAnimatedMeshAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  if (sMode == "AnimatedMeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (sMode == "AnimatedMeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return XII_SUCCESS;
}
