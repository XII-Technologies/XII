#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>
#include <ModelImporter2/ModelImporter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetDocument, 8, xiiRTTINoAllocator)
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

  XII_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));

  // the properties object can get invalidated by the CreateMeshFromFile() call
  pProp = GetProperties();

  range.BeginNextStep("Writing Result");

  if (!pProp->m_sDefaultSkeleton.IsEmpty())
  {
    desc.m_hDefaultSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(pProp->m_sDefaultSkeleton);
  }

  desc.Save(stream);

  return XII_SUCCESS;
}

xiiStatus xiiAnimatedMeshAssetDocument::CreateMeshFromFile(xiiAnimatedMeshAssetProperties* pProp, xiiMeshResourceDescriptor& desc)
{
  xiiProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  xiiStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return xiiStatus(xiiFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  xiiUniquePtr<xiiModelImporter2::Importer> pImporter = xiiModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return xiiStatus("No known importer for this file type.");

  xiiModelImporter2::ImportOptions opt;
  opt.m_sSourceFile               = sAbsFilename;
  opt.m_bImportSkinningData       = true;
  opt.m_bRecomputeNormals         = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents        = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput               = &desc;
  opt.m_MeshNormalsPrecision      = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision    = pProp->m_TexCoordPrecision;
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_MeshBoneWeightPrecision   = pProp->m_BoneWeightPrecision;
  opt.m_bNormalizeWeights         = pProp->m_bNormalizeWeights;
  // opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification      = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError  = pProp->m_uiMaxSimplificationError;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

  if (pImporter->Import(opt).Failed())
    return xiiStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    xiiMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      xiiMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  xiiMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

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
