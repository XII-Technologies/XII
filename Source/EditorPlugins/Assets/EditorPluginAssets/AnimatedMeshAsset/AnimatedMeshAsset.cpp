#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimatedMeshAssetDocument, 8, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimatedMeshAssetDocument::xiiAnimatedMeshAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiAnimatedMeshAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
}

xiiTransformStatus xiiAnimatedMeshAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
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

  return xiiStatus(XII_SUCCESS);
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
  opt.m_sSourceFile            = sAbsFilename;
  opt.m_bImportSkinningData    = true;
  opt.m_bRecomputeNormals      = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents     = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput            = &desc;
  opt.m_MeshNormalsPrecision   = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  //opt.m_RootTransform = CalculateTransformationMatrix(pProp);

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

  return xiiStatus(XII_SUCCESS);
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

xiiAnimatedMeshAssetDocumentGenerator::~xiiAnimatedMeshAssetDocumentGenerator() {}

void xiiAnimatedMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                          = "AnimatedMeshImport.WithMaterials";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Animated_Mesh.png";
  }

  {
    xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info.m_sName                          = "AnimatedMeshImport.NoMaterials";
    info.m_sOutputFileParentRelative      = baseOutputFile;
    info.m_sIcon                          = ":/AssetIcons/Animated_Mesh.png";
  }
}

xiiStatus xiiAnimatedMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiAnimatedMeshAssetDocument* pAssetDoc = xiiDynamicCast<xiiAnimatedMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiAnimatedMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  if (info.m_sName == "AnimatedMeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "AnimatedMeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return xiiStatus(XII_SUCCESS);
}
